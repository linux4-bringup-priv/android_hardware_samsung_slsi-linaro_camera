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

#ifndef EXYNOS_CAMERA_FRAME_FACTORY_H
#define EXYNOS_CAMERA_FRAME_FACTORY_H

#include "ExynosCameraFrameFactoryBase.h"
#include "ExynosCameraPipeFlite.h"
#include "ExynosCameraPipeGSC.h"
#include "ExynosCameraPipeSWMCSC.h"
#include "ExynosCameraPipeVRA.h"
#include "ExynosCameraPipeHFD.h"
#ifdef USE_SLSI_PLUGIN
#include "ExynosCameraPipePlugIn.h"
#endif
#ifdef SUPPORT_GMV
#include "ExynosCameraPipeGMV.h"
#endif
#include "ExynosCameraPipeJpeg.h"
#include "ExynosCameraPipeMultipleJpeg.h"

namespace android {

class ExynosCameraFrameFactory : public ExynosCameraFrameFactoryBase {
public:
    ExynosCameraFrameFactory()
    {
        m_init();
    }

    ExynosCameraFrameFactory(int cameraId,
                             ExynosCameraConfigurations *configurations,
                             ExynosCameraParameters *param,
                             cameraId_Info *camIdInfo)
        : ExynosCameraFrameFactoryBase(cameraId, configurations, param, camIdInfo)
    {
        m_init();
    }

public:
    virtual status_t        initPipes(void) = 0;
    virtual status_t        preparePipes(void) = 0;
    virtual status_t        startPipes(void) = 0;
    virtual status_t        stopPipes(void) = 0;
    virtual status_t        startInitialThreads(void) = 0;

    virtual enum NODE_TYPE  getNodeType(uint32_t pipeId);

    virtual ExynosCameraFrameSP_sptr_t createNewFrame(uint32_t frameCount, bool useJpegFlag = false) = 0;

    /* Helper functions for PipePP */
    virtual void            connectPPScenario(int pipeId, int scenario);
    virtual void            extControl(int pipeId, int scenario, int controlType, void *data);
    virtual void            startPPScenario(int pipeId, int scenario);
    virtual void            stopPPScenario(int pipeId, int scenario, bool suspendFlag = false);
    virtual int             getPPScenario(int pipeId);
    virtual bool            checkPlugin(int pipeId);

    virtual status_t        setParameter(uint32_t pipeId, int key, void *data);
    virtual status_t        getParameter(uint32_t pipeId, int key, void *data);
    virtual status_t        setParameter(uint32_t pipeId, int key, void *data, int scenario);
    virtual status_t        getParameter(uint32_t pipeId, int key, void *data, int scenario);

#ifdef SUPPORT_SENSOR_MODE_CHANGE
public:
    virtual status_t        initSensorPipe(void){return NO_ERROR;}
    virtual status_t        startSensorPipe(void){return NO_ERROR;}
    virtual status_t        stopSensorPipe(void){return NO_ERROR;}
    virtual status_t        stopAndWaitSensorPipeThread(void){return NO_ERROR;}
    virtual status_t        startSensorPipeThread(void){return NO_ERROR;}
#endif

protected:
    virtual status_t        m_setupConfig(void) = 0;
    virtual status_t        m_constructPipes(void) = 0;

    /* flite pipe setting */
    virtual status_t        m_initFlitePipe(int sensorW, int sensorH, uint32_t frameRate);

    virtual int             m_getSensorId(unsigned int nodeNum, unsigned int connectionMode,
                                          bool flagLeader, bool reprocessing,
                                          int sensorScenario = SENSOR_SCENARIO_NORMAL);

private:
    void                    m_init(void);
};

}; /* namespace android */

#endif
