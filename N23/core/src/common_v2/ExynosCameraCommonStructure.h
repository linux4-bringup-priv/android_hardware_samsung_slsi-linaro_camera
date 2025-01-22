/*
**
** Copyright 2019, Samsung Electronics Co. LTD
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

#ifndef EXYNOS_CAMERA_COMMON_STRUCTURE_H
#define EXYNOS_CAMERA_COMMON_STRUCTURE_H

#include <map>
#include <vector>
#include <cmath>
#include "ExynosRect.h"

#include "ExynosCameraCommonEnum.h"

struct DualTransitionInfo {
    enum DUAL_OPERATION_MODE dualOpMode;
    enum DUAL_OPERATION_MODE dualOpRepMode;
    // [0] is master camera
    std::vector<int32_t> cameraIds;
    enum DUAL_OPERATION_SENSORS dualOpSensor;
    enum DUAL_OPERATION_SENSORS dualOpRepSensor;
};

struct DualTransitionInfoCmp {
    bool operator() (float a, float b) const {
        // effective way to compare zoom ratio
        // eg A : 0.9901 -> 99
        //    B : 0.9900 -> 99
        //    => equal
        int intA = (a * 100.f);
        int intB = (b * 100.f);
        return intA < intB;
    }
};

typedef struct cameraIdInfo {
    cameraIdInfo() {
        serviceCameraId = -1;
        scenario = 0;
        camInfoIndex = -1;
        logicalSyncType = 0;
        for (int i = 0; i < MAX_NUM_SENSORS; i++)
            cameraId[i] = -1;
        numOfSensors = 0;
        sensorInfoCamIdx = MAIN_CAM;
    };

    int serviceCameraId;
    int scenario;
    int camInfoIndex;
    int logicalSyncType;
    int cameraId[MAX_NUM_SENSORS];
    int numOfSensors;
    int sensorInfoCamIdx;
    /*
     * key : lower_bound zoom ratio
     * value : dual transition information
     *
     * eg.
     *   zoom   = { opMode     , opMode(rep), camIds, sensors }
     *  [1.49 ] = { MASTER_ONLY, MASTER_ONLY, [0,  ], MAIN };
     *  [1.99 ] = { SYNC       , SYNC       , [0, 2]  MAIN_SUB};
     *  [3.99 ] = { SYNC       , SYNC       , [2, 0]  MAIN_SUB};
     *  [10.0 ] = { SLAVE_ONLY , SLAVE_ONLY , [2,  ]  SUB };
     *
     *  lookup -> result
     *  [1.0  ] -> [1.49]
     *  [1.49 ] -> [1.49]
     *  [1.5  ] -> [1.99]
     *  [1.51 ] -> [1.99]
     *  [2.0  ] -> [3.99]
     *  [3.9  ] -> [3.99]
     *  [3.99 ] -> [3.99]
     *  [4.0  ] -> [10.0]
     *  [4.1  ] -> [10.0]
     *
     * TODO: It can be created by JSON
     */
    std::map<float, DualTransitionInfo, DualTransitionInfoCmp> dualTransitionInfo;
} cameraId_Info;

typedef struct ExynosCameraSizeInfo {
    int pipeId; //For debuging
    ExynosRect rect;

    ExynosCameraSizeInfo() {
        pipeId = -1;
    }
} frame_size_info_t;

typedef std::map<frame_size_scenario_t, frame_size_info_t> FrameSizeInfoMap_t;

#endif /* EXYNOS_CAMERA_COMMON_STRUCTURE_H */
