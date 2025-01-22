/*
 * Copyright@ Samsung Electronics Co. LTD
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

/*!
 * \file      ExynosCameraTimeLogger.h
 * \brief     header file for ExynosCameraTimeLogger
 * \author    Teahyung, Kim(tkon.kim@samsung.com)
 * \date      2017/01/04
 *
 * <b>Revision History: </b>
 * - 2017/01/04 : Teahyung, Kim(tkon.kim@samsung.com) \n
 */

#ifndef EXYNOS_CAMERA_TIME_LOGGER_H
#define EXYNOS_CAMERA_TIME_LOGGER_H

#include "string.h"
#include <utils/Log.h>
#include <cutils/atomic.h>

#include "ExynosCameraUtils.h"
#include "ExynosCameraAutoTimer.h"
#include "ExynosCameraCommonInclude.h"
#include "ExynosCameraSingleton.h"
#include "ExynosCameraSensorInfoBase.h"

#define TIME_LOGGER_SIZE (1024 * 100) /* 100K * logger */
#define TIME_LOGGER_PATH "/data/vendor/camera/exynos_camera_time_logger_cam%d_%lld.csv"

#define TIME_LOGGER_INIT_BASE(logger, cameraId)          \
            ({ (logger)->init(cameraId); })
#define TIME_LOGGER_UPDATE_BASE(logger, cameraId, key, subkey, type, category, userData)  \
            ({ (logger)->update(cameraId, key, subkey, LOGGER_TYPE_ ## type, LOGGER_CATEGORY_ ## category, userData); })
#define TIME_LOGGER_SAVE_BASE(logger, cameraId)          \
            ({ (logger)->save(cameraId); })

#ifdef TIME_LOGGER_ENABLE
#define TIME_LOGGER_INIT(cameraId)          \
        ({                                  \
            extern ExynosCameraTimeLogger gLogger; \
            TIME_LOGGER_INIT_BASE(&gLogger, cameraId);    \
        })
/* you can remove "LOGGER_TYPE_", "LOGGER_CATEGORRY_" prefix */
#define TIME_LOGGER_UPDATE(cameraId, key, subkey, type, category, userData)  \
        ({                                                                \
            extern ExynosCameraTimeLogger gLogger; \
            TIME_LOGGER_UPDATE_BASE(&gLogger, cameraId, key, subkey, type, category, userData); \
        })
#define TIME_LOGGER_SAVE(cameraId)          \
        ({                                  \
            extern ExynosCameraTimeLogger gLogger; \
            TIME_LOGGER_SAVE_BASE(&gLogger, cameraId);         \
        })
#else
#define TIME_LOGGER_INIT(cameraId)
#define TIME_LOGGER_UPDATE(cameraId, key, subkey, type, category, userData) \
        ({LOGGER_TYPE_ ## type; LOGGER_CATEGORY_ ## category; })
#define TIME_LOGGER_SAVE(cameraId)
#endif

namespace android {

typedef enum LOGGER_TYPE {
    LOGGER_TYPE_BASE,
    LOGGER_TYPE_INTERVAL,
    LOGGER_TYPE_DURATION,
    LOGGER_TYPE_CUMULATIVE_CNT,
    LOGGER_TYPE_SUB_CUMULATIVE_CNT,
    LOGGER_TYPE_USER_DATA,
    LOGGER_TYPE_MAX,
} logger_type_t;

typedef enum LOGGER_CATEGORY {
    LOGGER_CATEGORY_BASE,
    /* for fixed purporse */
    LOGGER_CATEGORY_QBUF,
    LOGGER_CATEGORY_DQBUF,
    /* for genral purpose */
    LOGGER_CATEGORY_POINT0,
    LOGGER_CATEGORY_POINT1,
    LOGGER_CATEGORY_POINT2,
    LOGGER_CATEGORY_POINT3,
    LOGGER_CATEGORY_POINT4,
    LOGGER_CATEGORY_POINT5,
    LOGGER_CATEGORY_POINT6,
    LOGGER_CATEGORY_POINT7,
    LOGGER_CATEGORY_POINT8,
    LOGGER_CATEGORY_POINT9,
    /* for launching time purpose */
    LOGGER_CATEGORY_LAUNCHING_TIME_START,
    LOGGER_CATEGORY_OPEN_START,
    LOGGER_CATEGORY_OPEN_END,
    LOGGER_CATEGORY_INITIALIZE_START,
    LOGGER_CATEGORY_INITIALIZE_END,
    LOGGER_CATEGORY_FIRST_SET_PARAMETERS_START,
    LOGGER_CATEGORY_READ_ROM_THREAD_START,
    LOGGER_CATEGORY_READ_ROM_THREAD_END,
    LOGGER_CATEGORY_READ_ROM_THREAD_JOIN_START,
    LOGGER_CATEGORY_READ_ROM_THREAD_JOIN_END,
    LOGGER_CATEGORY_FIRST_SET_PARAMETERS_END,
    LOGGER_CATEGORY_CONFIGURE_STREAM_START,
    LOGGER_CATEGORY_FASTEN_AE_THREAD_START,
    LOGGER_CATEGORY_FASTEN_AE_THREAD_END,
    LOGGER_CATEGORY_FASTEN_AE_THREAD_JOIN_START,
    LOGGER_CATEGORY_FASTEN_AE_THREAD_JOIN_END,
    LOGGER_CATEGORY_STREAM_BUFFER_ALLOC_START,
    LOGGER_CATEGORY_STREAM_BUFFER_ALLOC_END,
    LOGGER_CATEGORY_FACTORY_CREATE_START,
    LOGGER_CATEGORY_FACTORY_CREATE_END,
    LOGGER_CATEGORY_FACTORY_CREATE_THREAD_JOIN_START,
    LOGGER_CATEGORY_FACTORY_CREATE_THREAD_JOIN_END,
    LOGGER_CATEGORY_CONFIGURE_STREAM_END,
    LOGGER_CATEGORY_PROCESS_CAPTURE_REQUEST_START,
    LOGGER_CATEGORY_SET_BUFFER_THREAD_START,
    LOGGER_CATEGORY_SET_BUFFER_THREAD_END,
    LOGGER_CATEGORY_SET_BUFFER_THREAD_JOIN_START,
    LOGGER_CATEGORY_SET_BUFFER_THREAD_JOIN_END,
    LOGGER_CATEGORY_FACTORY_START_THREAD_START,
    LOGGER_CATEGORY_DUAL_FACTORY_START_THREAD_START,
    LOGGER_CATEGORY_FACTORY_INIT_PIPES_START,
    LOGGER_CATEGORY_FACTORY_INIT_PIPES_END,
    LOGGER_CATEGORY_FACTORY_STREAM_START_START,
    LOGGER_CATEGORY_FACTORY_STREAM_START_END,
    LOGGER_CATEGORY_FACTORY_START_THREAD_END,
    LOGGER_CATEGORY_DUAL_FACTORY_START_THREAD_END,
    LOGGER_CATEGORY_PIPE_CREATE,
    LOGGER_CATEGORY_PIPE_SETUP,
    LOGGER_CATEGORY_PIPE_START,
    LOGGER_CATEGORY_PREVIEW_STREAM_THREAD,
    LOGGER_CATEGORY_RESULT_CALLBACK,
    LOGGER_CATEGORY_VIDEO_OPEN,
    LOGGER_CATEGORY_VIDEO_EOS,
    LOGGER_CATEGORY_VIDEO_S_INPUT,
    LOGGER_CATEGORY_VIDEO_S_FMT,
    LOGGER_CATEGORY_VIDEO_REQBUF,
    LOGGER_CATEGORY_VIDEO_STREAMON,
    LOGGER_CATEGORY_VIDEO_S_CTRL,
    LOGGER_CATEGORY_VIDEO_SENSOR_STREAMON,
    LOGGER_CATEGORY_VIDEO_STREAMOFF,
    LOGGER_CATEGORY_VIDEO_CLOSE,
    LOGGER_CATEGORY_FIRST_PREVIEW,
    LOGGER_CATEGORY_LAUNCHING_TIME_END,
    /* for streaming performance purpose */
    LOGGER_CATEGORY_STREAM_PERFORMANCE_START,
    LOGGER_CATEGORY_PROCESS_CAPTURE_REQUEST,
    LOGGER_CATEGORY_BLOCK_PROCESS_CAPTURE_REQUEST,
    LOGGER_CATEGORY_CREATE_PREVIEW_FRAME,
    LOGGER_CATEGORY_CREATE_CAPTURE_FRAME,
    LOGGER_CATEGORY_CREATE_INTERNAL_FRAME,
    LOGGER_CATEGORY_UPDATE_RESULT,
    LOGGER_CATEGORY_STREAM_PERFORMANCE_END,
    /* for closing time purpose */
    LOGGER_CATEGORY_CLOSING_TIME_START,
    LOGGER_CATEGORY_CLOSE_START,
    LOGGER_CATEGORY_CLOSE_END,
    LOGGER_CATEGORY_RELEASE_DEVICE_START,
    LOGGER_CATEGORY_RELEASE_DEVICE_END,
    LOGGER_CATEGORY_MONITOR_THREAD_STOP_START,
    LOGGER_CATEGORY_MONITOR_THREAD_STOP_END,
    LOGGER_CATEGORY_FLUSH_START,
    LOGGER_CATEGORY_FLUSH_END,
    LOGGER_CATEGORY_FRAME_CREATE_THREAD_STOP_END,
    LOGGER_CATEGORY_FACTORY_STREAM_STOP_START,
    LOGGER_CATEGORY_FACTORY_STREAM_STOP_END,
    LOGGER_CATEGORY_LIBRARY_DEINIT_END,
    LOGGER_CATEGORY_STREAM_THREAD_STOP_END,
    LOGGER_CATEGORY_REQUEST_FLUSH_START,
    LOGGER_CATEGORY_REQUEST_FLUSH_END,
    LOGGER_CATEGORY_BUFFER_RESET_START,
    LOGGER_CATEGORY_BUFFER_RESET_END,
    LOGGER_CATEGORY_WAIT_PROCESS_CAPTURE_REQUEST_START,
    LOGGER_CATEGORY_WAIT_PROCESS_CAPTURE_REQUEST_END,
    LOGGER_CATEGORY_DESTRUCTOR_START,
    LOGGER_CATEGORY_DESTRUCTOR_END,
    LOGGER_CATEGORY_PRE_DESTRUCTOR_END,
    LOGGER_CATEGORY_BUFFER_SUPPLIER_DEINIT_START,
    LOGGER_CATEGORY_BUFFER_SUPPLIER_DEINIT_END,
    LOGGER_CATEGORY_FACTORY_DESTROY_START,
    LOGGER_CATEGORY_FACTORY_DESTROY_END,
    LOGGER_CATEGORY_BUFFER_SUPPLIER_DEINIT_JOIN_START,
    LOGGER_CATEGORY_BUFFER_SUPPLIER_DEINIT_JOIN_END,
    LOGGER_CATEGORY_POST_DESTRUCTOR_END,
    LOGGER_CATEGORY_CLOSING_TIME_END,
    LOGGER_CATEGORY_MAX,
} logger_category_t;

/*
 * Class ExynosCameraTimeLogger
 * ExynosCameraTimeLogger is the time logging class for profiling performance.
 * This class cannot support concurrent processing of timelogging for same logger
 * (same logger condition : same pipeLine && same logger_type && same logger_category)
 */
class ExynosCameraTimeLogger
{
public:

    typedef struct timeLogger {
        pid_t tid;
        uint64_t timeStamp;     /* ns : logging time */
        int cameraId;
        uint64_t key;
        uint32_t pipeId;
        LOGGER_TYPE type;
        LOGGER_CATEGORY category;
        uint32_t calTime;       /* us : cacluated time(duration, interval..) */
    } timeLogger_t;

    /* memset all logger information */
    status_t init(int cameraId);

    /*
     * update information
     *  @cameraId : (mandatory)
     *  @key : (mandatory) like buffer index or frameCount
     *  @pipeId : (optional)
     *  @type : (mandatory)
     *  @category : (mandatory)
     *  @userData : (mandatory) Always true in case of INTERVAL type
     * eg.
     *  <interval>
     *    getBuffer(&node);
     *    update(backCameraId, frameCount, PIPE_MCSC1, LOGGER_TYPE_INTERVAL, LOGGER_CATEGORY_DQBUF, true);
     *
     *  <duration>
     *    update(backCameraId, frameCount, PIPE_MCSC1, LOGGER_TYPE_DURATION, LOGGER_CATEGORY_DQBUF, true);
     *    getBuffer(&node);
     *    update(backCameraId, frameCount, PIPE_MCSC1, LOGGER_TYPE_DURATION, LOGGER_CATEGORY_DQBUF, false);
     */
    status_t update(int cameraId, uint64_t key, uint32_t pipeId, LOGGER_TYPE type, LOGGER_CATEGORY category, uint64_t userData);

    /*
     * save all information to file
     */
    status_t save(int cameraId);

    /*
     * check define condition to do logging
     */
    bool checkCondition(LOGGER_CATEGORY category);

    friend class ExynosCameraSingleton<ExynosCameraTimeLogger>;
    ExynosCameraTimeLogger();
    virtual ~ExynosCameraTimeLogger();

private:
    bool                    m_stopFlag[CAMERA_ID_MAX];
    bool                    m_firstCheckFlag[CAMERA_ID_MAX][LOGGER_TYPE_MAX];
    ExynosCameraDurationTimer   m_timer[CAMERA_ID_MAX][LOGGER_TYPE_MAX][LOGGER_CATEGORY_MAX][MAX_PIPE_NUM];
    uint32_t                m_count[CAMERA_ID_MAX];
    int32_t                 m_bufferIndex[CAMERA_ID_MAX];
    timeLogger_t            *m_buffer[CAMERA_ID_MAX];
    char                    m_name[EXYNOS_CAMERA_NAME_STR_SIZE];
    char                    *m_typeStr[LOGGER_TYPE_MAX];
    char                    *m_categoryStr[LOGGER_CATEGORY_MAX];
    uint64_t                 m_firstTimestamp;
    uint64_t                 m_lastTimestamp;
};
};
#endif //EXYNOS_CAMERA_TIME_LOGGER_H
