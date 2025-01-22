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

#ifndef EXYNOS_CAMERA_RESOURCE_MANAGER_H__
#define EXYNOS_CAMERA_RESOURCE_MANAGER_H__

#include <utils/RefBase.h>
#include <utils/Errors.h>

#include "ExynosCameraCommonDefine.h"

namespace android {

class ExynosCameraConfigurations;
class ExynosCameraParameters;
class ExynosCameraStreamManager;
class ExynosCameraRequestManager;
class ExynosCameraBufferSupplier;
class ExynosCameraFrameFactory;
class ExynosCameraActivityControl;
class ExynosCameraFrameSelector;
class ExynosCameraMetadataConverter;
class ExynosCameraFrameFactory;
class ExynosCameraFrameManager;
class ExynosCameraBufferSupplier;
class ExynosCameraStreamManager;

#define RESOURCE_MAX 5

class ExynosCameraResourceManager : public virtual RefBase {
public:
    typedef struct captureProperty {
        ExynosCameraFrameSelector* frameSelector[CAMERA_ID_MAX];
        ExynosCameraFrameSelector* zslFrameSelector;
    } capture_property_t;

    typedef struct bufferProperty {
        ExynosCameraBufferSupplier* bufferSupplier;
        ExynosCameraIonAllocator*   ionAllocator;
    } buffer_property_t;

    typedef struct resources {
        ExynosCameraConfigurations *configuration;
        ExynosCameraParameters *parameters[CAMERA_ID_MAX];
        ExynosCameraFrameFactory *frameFactories[FRAME_FACTORY_TYPE_MAX];
        ExynosCameraActivityControl *activityControls[CAMERA_ID_MAX];
        ExynosCameraRequestManager* requestMgr;
        ExynosCameraStreamManager* streamMgr;
        capture_property_t captureProperty;
        buffer_property_t  bufferProperty;
        bool cloned;
    } resources_t;

public:
    ExynosCameraResourceManager(int cameraId, char* cameraName);
    ~ExynosCameraResourceManager();

    status_t initResources(cameraId_Info* pCamIdInfo,
                                ExynosCameraConfigurations** pConfigurations,
                                ExynosCameraParameters* pParameters[CAMERA_ID_MAX],
                                ExynosCameraMetadataConverter** pMetadataConverter);

    status_t deInitResources(cameraId_Info* pCamIdInfo,
                                    ExynosCameraConfigurations** pConfigurations,
                                    ExynosCameraParameters* pParameters[CAMERA_ID_MAX],
                                    ExynosCameraMetadataConverter** pMetadataConverter);

    status_t initManagerResources(cameraId_Info* pCamIdInfo,
                                            ExynosCameraRequestManager** pRequestMgr,
                                            ExynosCameraStreamManager** pStreamMgr,
                                            ExynosCameraFrameManager** pFrameMgr);

    ExynosCameraRequestManager* getRequestManager(int refIndex = 0);

    status_t initBufferSupplier(ExynosCameraBufferSupplier* pBufferSupplier, ExynosCameraIonAllocator* pIonAllocator);
    status_t deInitBufferSupplier(ExynosCameraBufferSupplier** pBufferSupplier, ExynosCameraIonAllocator** pIonAllocator);

    status_t initCaptureProperty(cameraId_Info* pCamIdInfo, capture_property_t* property);

    status_t registerFrameFactory(ExynosCameraFrameFactory** pFrameFactories);

    resources_t* getResource(int refIndex = 0);

    status_t cloneResources(int refIndex);

    status_t deleteResources(int refIndex);

    int      getRefCount() { return m_refCount.getCount(); }

    void         dumpResources();

private:
    void           m_init();
    void           m_delete();
    void           m_delete(int refIndex);

    int            m_getRefIndex();

private:
    ExynosCameraMetadataConverter   *m_metadataConverter;
    ExynosCameraFrameManager        *m_frameMgr;

    resources_t                     m_resources[RESOURCE_MAX];

    int                             m_cameraId;
    char                            m_name[EXYNOS_CAMERA_NAME_STR_SIZE];

    ExynosCameraCounter             m_refCount;

};

};

#endif //EXYNOS_CAMERA_RESOURCE_MANAGER_H__

