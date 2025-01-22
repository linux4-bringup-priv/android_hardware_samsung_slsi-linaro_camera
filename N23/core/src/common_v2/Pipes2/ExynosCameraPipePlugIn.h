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

#ifndef EXYNOS_CAMERA_PIPE_PLUG_IN_H
#define EXYNOS_CAMERA_PIPE_PLUG_IN_H

#include <unordered_map>

#include "ExynosCameraArgThread.h"
#include "ExynosCameraSWPipe.h"
#include "ExynosCameraFactoryPlugIn.h"

namespace android {

using namespace std;

class ExynosCameraPipePlugIn : public ExynosCameraSWPipe {
private:
    class PlugInObject : public RefBase {
    public:
        PlugInObject(ExynosCameraPlugInSP_sptr_t plugIn, ExynosCameraPlugInConverterSP_sptr_t plugInConverter)
        {
            m_plugIn = plugIn;
            m_plugInConverter = plugInConverter;
        }

        virtual ~PlugInObject(){}

    public:
        ExynosCameraPlugInSP_sptr_t m_plugIn;
        ExynosCameraPlugInConverterSP_sptr_t m_plugInConverter;
    };

    typedef sp<PlugInObject>  PlugInSP_sptr_t; /* single ptr */

    // All keys are scenario (enum PlugInScenario)
    typedef unordered_map<int, PlugInSP_sptr_t>  PlugInMap_t;
    typedef sp<ExynosCameraArgThread<ExynosCameraPipePlugIn>> Thread_t;
    typedef unordered_map<int, Thread_t> ThreadMap_t;
    typedef unordered_map<int, int> PlugInStartCountMap_t;

    enum MultiLibType {
        /*
         * STREAM: (default)
         * After finishing every m_run(),
         * each plugIn's resources are still preserved.
         */
        STREAM,

        /*
         * ONESHOT:
         * After finishing every m_run(),
         * the plugIn's resources would be released.
         * (call PLUGIN_PARAMETER_KEY_STOP)
         * But it would be working at last frame
         *  eg. frame->getFrameIndex() == frame->getMaxFrameIndex()
         */
        ONESHOT,
    };

public:
    ExynosCameraPipePlugIn()
    {
        m_init(NULL);
    }

    ExynosCameraPipePlugIn(
        int cameraId,
        ExynosCameraConfigurations *configurations,
        ExynosCameraParameters *obj_param,
        bool isReprocessing,
        int32_t *nodeNums,
        cameraId_Info *camIdInfo,
        int scenario = 0, bool supportMultiLibrary = false) : ExynosCameraSWPipe(cameraId, configurations, obj_param, isReprocessing, nodeNums, camIdInfo)
    {
        m_scenario = scenario;
        m_supportMultiLibrary = supportMultiLibrary;
        m_init(nodeNums);
    }

    virtual ~ExynosCameraPipePlugIn();

    virtual status_t        create(int32_t *sensorIds = NULL);
    virtual status_t        destroy(void);
    virtual status_t        start(void);
    virtual status_t        stop(void);
    virtual status_t        setupPipe(Map_t *map);
    virtual status_t        setParameter(int key, void *data);
    virtual status_t        getParameter(int key, void *data);
    virtual status_t        setParameter(int key, void *data, int scenario);
    virtual status_t        getParameter(int key, void *data, int scenario);
    virtual void            dump(void);

protected:
    typedef enum handle_status {
        PLUGIN_ERROR = -1,
        PLUGIN_NO_ERROR,
        PLUGIN_SKIP,
    } handle_status_t;
    virtual status_t                m_run(void);
    virtual handle_status_t         m_reorderFramebyFrameIndex(__unused ExynosCameraFrameSP_sptr_t frame,
                                                                    __unused PlugInScenario scenario);
    virtual handle_status_t         m_handleFrameBefore(__unused ExynosCameraFrameSP_dptr_t frame,
                                                        __unused ExynosCameraPlugInSP_sptr_t plugIn,
                                                        __unused Map_t *map,
                                                        __unused int scenario);
    virtual handle_status_t         m_handleFrameAfter(__unused ExynosCameraFrameSP_dptr_t frame,
                                                       __unused ExynosCameraPlugInSP_sptr_t plugIn,
                                                       __unused Map_t *map,
                                                       __unused int scenario);
private:
    void                    m_init(__unused int32_t *nodeNums);
    status_t                m_create(int scenario, int32_t *sensorIds = NULL);
    status_t                m_start(ExynosCameraPlugInSP_sptr_t plugIn);
    status_t                m_stop(ExynosCameraPlugInSP_sptr_t plugIn);
    status_t                m_setup(__unused Map_t *map);
    status_t                m_setupPipe(ExynosCameraPlugInSP_sptr_t plugIn, ExynosCameraPlugInConverterSP_sptr_t plugInConverter, Map_t *map);
    status_t                m_setParameter(ExynosCameraPlugInSP_sptr_t plugIn, int key, void *data, int scenario);
    status_t                m_getParameter(ExynosCameraPlugInSP_sptr_t plugIn, int key, void *data, int scenario);

    /* On Perframe, acessing plugInMap should be protected by mutex lock */
    ExynosCameraPlugInSP_sptr_t  m_getPlugIn(int key);
    status_t                m_pushPlugInItem(int key, PlugInSP_sptr_t item);
    status_t                m_popPlugInItem(int key, PlugInSP_sptr_t *item);
    status_t                m_getPlugInItem(int key, PlugInSP_sptr_t *item);
    bool                    m_findPlugInItem(int key);

    status_t                m_updateSetCmd(__unused ExynosCameraPlugInSP_sptr_t plugIn, int cmd, void *data, int scenario);
    status_t                m_updateGetCmd(__unused ExynosCameraPlugInSP_sptr_t plugIn, int cmd, void *data, int scenario);

    status_t                m_setScenario(int scenario);
    status_t                m_getScenario(int& scenario);

    bool                    m_loadPlugInThreadFunc(void *data);
    bool                    m_startPlugInThreadFunc(void *data);
    status_t                m_copyMeta(ExynosCameraFrameSP_sptr_t frame, int dstCnt);
    void                    m_joinPlugInThread(int scenario);

private:
    ExynosCameraFactoryPlugIn           *m_globalPlugInFactory;
    ExynosCameraPlugInSP_sptr_t          m_plugIn;
    ExynosCameraPlugInConverterSP_sptr_t m_plugInConverter;
    int                                  m_scenario;

    // For multilibrary
    PlugInMap_t                          m_plugInMap;
    mutable Mutex                        m_plugInMapLock;
    mutable Mutex                        m_plugInStartLock;
    bool                                 m_supportMultiLibrary;
    enum MultiLibType                    m_multiLibraryType;
    ThreadMap_t                          m_loaderPlugInThreadMap;
    ThreadMap_t                          m_startPlugInThreadMap;
    PlugInStartCountMap_t                m_startPlugInCounterMap;
    Map_t                                m_plugInSetupMap;
    map<int, ExynosCameraFrameSP_sptr_t> m_frameReorderMap;
    int                                  m_runningFrameCount;
    int                                  m_runningMetaFrameCount;
};

}; /* namespace android */

#endif
