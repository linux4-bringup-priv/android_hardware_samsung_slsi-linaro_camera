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
#define LOG_TAG "ExynosCameraResourceManager"

#include "ExynosCameraConfigurations.h"
#include "ExynosCameraParameters.h"
#include "ExynosCameraResourceManager.h"
#include "ExynosCameraStreamManager.h"
#include "ExynosCameraBufferSupplier.h"
#include "ExynosCameraRequestManager.h"

#include "ExynosCameraResourceManager.h"

namespace android {

#define ENABLE_RESOURCE_MGR_DEBUG_PRINT

#ifdef ENABLE_RESOURCE_MGR_DEBUG_PRINT
#define RESOURCE_DEBUG_DUMP_LOG CLOGD
#else
#define RESOURCE_DEBUG_DUMP_LOG CLOGV
#endif


ExynosCameraResourceManager::ExynosCameraResourceManager(int cameraId, char* cameraName)
{
    m_cameraId = cameraId;

    snprintf(m_name, sizeof(m_name), "resourceMgr_%s", cameraName);

    m_init();

    CLOGD("[%s] create ExynosCameraResourceManager", m_name);
}

ExynosCameraResourceManager::~ExynosCameraResourceManager()
{
    m_delete();

    CLOGD("detroy ExynosCameraResourceManager");
}

void ExynosCameraResourceManager::m_init()
{
    m_metadataConverter = nullptr;

    for (int i = 0; i < RESOURCE_MAX; i++) {

        m_resources[i].configuration = nullptr;

        for(int j = 0; j < CAMERA_ID_MAX; j++) {
            m_resources[i].parameters[j] = nullptr;
            m_resources[i].activityControls[j] = nullptr;
            m_resources[i].captureProperty.frameSelector[j] = nullptr;
        }

        for(int j = 0; j < FRAME_FACTORY_TYPE_MAX; j++) {
            m_resources[i].frameFactories[j] = nullptr;
        }

        m_resources[i].captureProperty.zslFrameSelector = nullptr;

        m_resources[i].streamMgr = nullptr;

        m_resources[i].requestMgr = nullptr;

        m_resources[i].bufferProperty.bufferSupplier = nullptr;
        m_resources[i].bufferProperty.ionAllocator = nullptr;

        m_resources[i].cloned = false;
    }

    m_frameMgr = nullptr;

}

void ExynosCameraResourceManager::m_delete()
{

    SAFE_DELETE(m_metadataConverter);

    for (int i = 0; i < RESOURCE_MAX; i++) {

        if (m_resources[i].cloned)
            continue;

        m_delete(i);
    }

    SAFE_DELETE(m_frameMgr);
}

void ExynosCameraResourceManager::m_delete(int refIndex)
{
    SAFE_DELETE(m_resources[refIndex].configuration);

    for(int j = 0; j < CAMERA_ID_MAX; j++) {
        if (m_resources[refIndex].parameters[j] != nullptr) {
            SAFE_DELETE(m_resources[refIndex].parameters[j]);
        }

        if (m_resources[refIndex].activityControls[j] != nullptr) {
            //released by parameters destructor
            //SAFE_DELETE(m_resources[refIndex].activityControls[j]);
            m_resources[refIndex].activityControls[j] = nullptr;
        }

        if (m_resources[refIndex].captureProperty.frameSelector[j] != nullptr) {
            SAFE_DELETE(m_resources[refIndex].captureProperty.frameSelector[j]);
        }
    }

    SAFE_DELETE(m_resources[refIndex].captureProperty.zslFrameSelector);

    SAFE_DELETE(m_resources[refIndex].streamMgr);

    SAFE_DELETE(m_resources[refIndex].requestMgr);

    SAFE_DELETE(m_resources[refIndex].bufferProperty.bufferSupplier);
    SAFE_DELETE(m_resources[refIndex].bufferProperty.ionAllocator);

}

status_t ExynosCameraResourceManager::initResources(cameraId_Info* pCamIdInfo,
                                                        ExynosCameraConfigurations** pConfigurations,
                                                        ExynosCameraParameters* pParameters[CAMERA_ID_MAX],
                                                        ExynosCameraMetadataConverter** pMetadataConverter)
{
    status_t ret = NO_ERROR;

    int refIndex;

    m_refCount.incCount();

    if (m_getRefIndex() >= RESOURCE_MAX) {
        m_refCount.setCount(1);
    }

    CLOGD("m_refCount(%d)", m_getRefIndex());

    if (m_getRefIndex() >= RESOURCE_MAX) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):refCount(%d) is larger than max", __FUNCTION__, __LINE__, m_refCount.getCount());
    }

    refIndex = m_getRefIndex();

    if (*pConfigurations != NULL) {
        CLOGW("pConfigurations is already created");
    }

    if (pParameters[m_cameraId] != NULL) {
        CLOGW("pParameters is already created");
    }

    if (*pMetadataConverter != NULL) {
        CLOGW("pMetadataConverter is already created");
    }

    m_resources[refIndex].configuration = new ExynosCameraConfigurations(pCamIdInfo);

    /* Create related classes */
    switch (pCamIdInfo->scenario) {
#ifdef USE_DUAL_CAMERA
    case SCENARIO_DUAL_REAR_ZOOM:
    case SCENARIO_DUAL_REAR_PORTRAIT:
    case SCENARIO_DUAL_FRONT_PORTRAIT:
        for (int i = 1; i < pCamIdInfo->numOfSensors; i++) {
            m_resources[refIndex].parameters[pCamIdInfo->cameraId[i]] = new ExynosCameraParameters(pCamIdInfo, i, m_resources[refIndex].configuration);
            if (m_resources[refIndex].parameters[pCamIdInfo->cameraId[i]] != nullptr) {
                m_resources[refIndex].activityControls[pCamIdInfo->cameraId[i]] = m_resources[refIndex].parameters[pCamIdInfo->cameraId[i]]->getActivityControl();
                if (m_resources[refIndex].activityControls[pCamIdInfo->cameraId[i]] == nullptr) {
                    android_printAssert(NULL, LOG_TAG, "Failed to create activityControls[%d]", pCamIdInfo->cameraId[i]);
                }

                pParameters[pCamIdInfo->cameraId[i]] = m_resources[refIndex].parameters[pCamIdInfo->cameraId[i]];
            } else {
                android_printAssert(NULL, LOG_TAG, "Failed to create parameters[%d]", pCamIdInfo->cameraId[i]);
            }
        }
        /* No break: m_parameters[0] is same with normal */
#endif
    case SCENARIO_NORMAL:
    default:
        m_resources[refIndex].parameters[m_cameraId] = new ExynosCameraParameters(pCamIdInfo, MAIN_CAM, m_resources[refIndex].configuration);
        if (m_resources[refIndex].parameters[m_cameraId] != nullptr) {
            m_resources[refIndex].activityControls[m_cameraId] = m_resources[refIndex].parameters[m_cameraId]->getActivityControl();
            if (m_resources[refIndex].activityControls[m_cameraId] == nullptr) {
                android_printAssert(NULL, LOG_TAG, "Failed to create activityControls[%d]", m_cameraId);
            }

            pParameters[m_cameraId] = m_resources[refIndex].parameters[m_cameraId];
        } else {
            android_printAssert(NULL, LOG_TAG, "Failed to create parameters[%d]", m_cameraId);
        }
        break;
    }

    for (int i = 0; i < pCamIdInfo->numOfSensors; i++) {
        int cameraId = pCamIdInfo->cameraId[i];
        m_resources[refIndex].configuration->setParameters(
                cameraId, m_resources[refIndex].parameters[cameraId]);
    }

    m_metadataConverter = new ExynosCameraMetadataConverter(pCamIdInfo, m_resources[refIndex].configuration, m_resources[refIndex].parameters);

    *pConfigurations = m_resources[refIndex].configuration;
    *pMetadataConverter = m_metadataConverter;

    m_resources[refIndex].cloned = false;

    return ret;
}

status_t ExynosCameraResourceManager::deInitResources(cameraId_Info* pCamIdInfo,
                                                            ExynosCameraConfigurations** pConfigurations,
                                                            ExynosCameraParameters* pParameters[CAMERA_ID_MAX],
                                                            ExynosCameraMetadataConverter** pMetadataConverter)
{
    if (*pMetadataConverter == m_metadataConverter) {
        CLOGD("Managing same resource of metadataConverter");
    } else {
        CLOGW("Managing different resource of metadataConverter");
    }

    *pConfigurations = nullptr;

    switch (pCamIdInfo->scenario) {
#ifdef USE_DUAL_CAMERA
    case SCENARIO_DUAL_REAR_ZOOM:
    case SCENARIO_DUAL_REAR_PORTRAIT:
    case SCENARIO_DUAL_FRONT_PORTRAIT:
        for (int i = 1; i < pCamIdInfo->numOfSensors; i++) {
            pParameters[pCamIdInfo->cameraId[i]] = nullptr;
        }
        /* No break: m_parameters[0] is same with normal */
#endif
    case SCENARIO_NORMAL:
    default:
        pParameters[m_cameraId] = nullptr;
        break;
    }

    SAFE_DELETE(m_metadataConverter);
    *pMetadataConverter = nullptr;

    return NO_ERROR;
}

ExynosCameraResourceManager::resources_t* ExynosCameraResourceManager::getResource(int refIndex)
{
#if 0
    if (refIndex > m_getRefIndex()) {
        android_printAssert(NULL, LOG_TAG, "cameraSessionId(%d) is larger than refIndex(%d)", refIndex, m_getRefIndex());
    }
#endif

    if (refIndex < 1) {
        refIndex = m_getRefIndex();
    }

    return &m_resources[refIndex];
}

status_t ExynosCameraResourceManager::cloneResources(int refIndex)
{
    int prevRefIndex = m_refCount.getCount();


    m_refCount.setCount(refIndex);

    if (m_getRefIndex() >= RESOURCE_MAX) {
        m_refCount.setCount(1);
    }

    CLOGD("m_refCount(%d), prevRefIndex(%d)", m_getRefIndex(), prevRefIndex);

    m_resources[m_getRefIndex()].configuration = m_resources[prevRefIndex].configuration;

    for(int j = 0; j < CAMERA_ID_MAX; j++) {
        m_resources[m_getRefIndex()].parameters[j] = m_resources[prevRefIndex].parameters[j];
        m_resources[m_getRefIndex()].activityControls[j] = m_resources[prevRefIndex].activityControls[j];
        m_resources[m_getRefIndex()].captureProperty.frameSelector[j] = m_resources[prevRefIndex].captureProperty.frameSelector[j];
    }

#if 0
    for(int j = 0; j < FRAME_FACTORY_TYPE_MAX; j++) {
        m_resources[m_getRefIndex()].frameFactories[j] = m_resources[prevRefIndex].frameFactories[j];
    }
#endif

    m_resources[m_getRefIndex()].captureProperty.zslFrameSelector = m_resources[prevRefIndex].captureProperty.zslFrameSelector;

    m_resources[m_getRefIndex()].streamMgr = m_resources[prevRefIndex].streamMgr;

    m_resources[m_getRefIndex()].requestMgr = m_resources[prevRefIndex].requestMgr;

    m_resources[m_getRefIndex()].cloned = true;

    return NO_ERROR;
}


status_t ExynosCameraResourceManager::deleteResources(int refIndex)
{
    CLOGD("Delete resources[%d]", refIndex);

    m_delete(refIndex);

    return NO_ERROR;
}

status_t ExynosCameraResourceManager::registerFrameFactory(ExynosCameraFrameFactory** pFrameFactories)
{
    for(int i = 0; i < FRAME_FACTORY_TYPE_MAX; i++) {
        m_resources[m_getRefIndex()].frameFactories[i] = pFrameFactories[i];
    }

#ifdef ENABLE_RESOURCE_MGR_DEBUG_PRINT
    dumpResources();
#endif

    return NO_ERROR;
}

status_t ExynosCameraResourceManager::initManagerResources(cameraId_Info* pCamIdInfo,
                                                                    ExynosCameraRequestManager** pRequestMgr,
                                                                    ExynosCameraStreamManager** pStreamMgr,
                                                                    ExynosCameraFrameManager** pFrameMgr)
{
    status_t ret = NO_ERROR;

    if (pStreamMgr != nullptr) {
        m_resources[m_getRefIndex()].streamMgr = new ExynosCameraStreamManager(m_cameraId);
        CLOGD("[S(%d)] Create streamManager", m_getRefIndex());

        *pStreamMgr = m_resources[m_getRefIndex()].streamMgr;
    } else {
        CLOGW2("Do not create streamManager");
    }

    if (pRequestMgr != nullptr) {
        m_resources[m_getRefIndex()].requestMgr = new ExynosCameraRequestManager(pCamIdInfo, m_resources[m_getRefIndex()].configuration);
        CLOGD("[S(%d)] Create requestManager", m_getRefIndex());

        *pRequestMgr = m_resources[m_getRefIndex()].requestMgr;
    } else {
        CLOGD2("[S(%d)] Do not create requestManager", m_getRefIndex());
    }

    if (pFrameMgr != nullptr) {

        if (m_frameMgr == nullptr) {
            m_frameMgr = new ExynosCameraFrameManager("FRAME MANAGER", m_cameraId, FRAMEMGR_OPER::SLIENT);
            CLOGD("[S(%d)] Create frameManager", m_getRefIndex());
        } else {
            CLOGW2("[S(%d)] frameManager is already created", m_getRefIndex());
        }

        *pFrameMgr = m_frameMgr;
    } else {
        CLOGD2("[S(%d)] Do not create frameManager", m_getRefIndex());
    }

    return ret;
}

ExynosCameraRequestManager* ExynosCameraResourceManager::getRequestManager(int cameraSessionId)
{
#if 0
    if (cameraSessionId > m_getRefIndex()) {
        android_printAssert(NULL, LOG_TAG, "cameraSessionId(%d) is larger than refIndex(%d)", cameraSessionId, m_getRefIndex());
    }
#endif

    if (cameraSessionId < 1) {
        cameraSessionId = m_getRefIndex();
    }

    if (m_resources[cameraSessionId].requestMgr == nullptr) {
        android_printAssert(NULL, LOG_TAG, "requestManager of cameraSessionId(%d) is NULL", cameraSessionId);
    }

    return m_resources[cameraSessionId].requestMgr;
}

status_t ExynosCameraResourceManager::initBufferSupplier(ExynosCameraBufferSupplier* pBufferSupplier, ExynosCameraIonAllocator* pIonAllocator)
{
    status_t ret = NO_ERROR;

    if (pBufferSupplier != nullptr) {
        m_resources[m_getRefIndex()].bufferProperty.bufferSupplier = pBufferSupplier;
    } else {
        android_printAssert(NULL, LOG_TAG, "Do not add bufferSupplier");
    }

    if (pIonAllocator != nullptr) {
        m_resources[m_getRefIndex()].bufferProperty.ionAllocator = pIonAllocator;
    } else {
        android_printAssert(NULL, LOG_TAG, "Do not add ionAllocator");
    }

    return ret;
}

status_t ExynosCameraResourceManager::deInitBufferSupplier(ExynosCameraBufferSupplier** pBufferSupplier, ExynosCameraIonAllocator** pIonAllocator)
{
    status_t ret = NO_ERROR;

    SAFE_DELETE(*pBufferSupplier);
    m_resources[m_getRefIndex()].bufferProperty.bufferSupplier = nullptr;
    *pBufferSupplier = nullptr;

    SAFE_DELETE(*pIonAllocator);
    m_resources[m_getRefIndex()].bufferProperty.ionAllocator = nullptr;
    *pIonAllocator = nullptr;

    return ret;
}

status_t ExynosCameraResourceManager::initCaptureProperty(cameraId_Info* pCamIdInfo, capture_property_t* property)
{
    CLOGD2("");

    status_t ret = NO_ERROR;

    int refIndex = m_getRefIndex();

    if (m_resources[refIndex].bufferProperty.bufferSupplier == nullptr || m_frameMgr == nullptr) {
        android_printAssert(NULL, LOG_TAG, "m_bufferSupplier or m_frameMgr is NULL");
    }

    if (m_resources[refIndex].parameters[m_cameraId]->getActivityControl() == nullptr) {
        android_printAssert(NULL, LOG_TAG, "getActivityControl is NULL");
    }

    for (int i = 0; i < pCamIdInfo->numOfSensors; i++) {
        m_resources[refIndex].captureProperty.frameSelector[pCamIdInfo->cameraId[i]] = new ExynosCameraFrameSelector(pCamIdInfo->cameraId[i],
                                                                                        m_resources[refIndex].configuration,
                                                                                        m_resources[refIndex].parameters[pCamIdInfo->cameraId[i]],
                                                                                        m_resources[refIndex].bufferProperty.bufferSupplier,
                                                                                        m_frameMgr);

        property->frameSelector[pCamIdInfo->cameraId[i]] = m_resources[refIndex].captureProperty.frameSelector[pCamIdInfo->cameraId[i]];
    }

    m_resources[refIndex].captureProperty.zslFrameSelector = new ExynosCameraFrameSelector(m_cameraId,
                                                                m_resources[refIndex].configuration,
                                                                m_resources[refIndex].parameters[m_cameraId],
                                                                m_resources[refIndex].bufferProperty.bufferSupplier,
                                                                m_frameMgr);

    property->zslFrameSelector = m_resources[refIndex].captureProperty.zslFrameSelector;


    return ret;
}

int ExynosCameraResourceManager::m_getRefIndex()
{
    CLOGV("refIndex(%d)", m_refCount.getCount());

    return m_refCount.getCount();
}

void ExynosCameraResourceManager::dumpResources()
{
    int i, j;

    for(i = 0; i <= m_refCount.getCount(); i++) {

        RESOURCE_DEBUG_DUMP_LOG("[%d] configuration [%p]", i, m_resources[i].configuration);

        for(j = 0; j < CAMERA_ID_MAX; j++) {
            if (m_resources[i].parameters[j] == nullptr) {
                //RESOURCE_DEBUG_DUMP_LOG("[%d] parameters[%d] [%s]", i, j, "NULL");
            } else {
                RESOURCE_DEBUG_DUMP_LOG("[%d] parameters[%d] [%p]", i, j, m_resources[i].parameters[j]);
            }
        }

        for(j = 0; j < CAMERA_ID_MAX; j++) {
            if (m_resources[i].activityControls[j] == nullptr) {
                //RESOURCE_DEBUG_DUMP_LOG("[%d] activityControls[%d] [%s]", i, j, "NULL");
            } else {
                RESOURCE_DEBUG_DUMP_LOG("[%d] activityControls[%d] [%p]", i, j, m_resources[i].activityControls[j]);
            }
        }

        for(j = 0; j < FRAME_FACTORY_TYPE_MAX; j++) {
            if (m_resources[i].frameFactories[j] == nullptr) {
                //RESOURCE_DEBUG_DUMP_LOG("[%d] frameFactories[%d] [%s]", i, j, "NULL");
            } else {
                RESOURCE_DEBUG_DUMP_LOG("[%d] frameFactories[%d] [%p]", i, j, m_resources[i].frameFactories[j]);
            }
        }

        //RESOURCE_DEBUG_DUMP_LOG("[%d] bufferSupplier [%p]", i, m_resources[i].bufferProperty.bufferSupplier);
    }
}

};

