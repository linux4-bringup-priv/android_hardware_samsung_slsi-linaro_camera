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

#define LOG_TAG "ExynosCameraLogManager"

#include <list>
#include <utils/Timers.h>
#include <utils/String8.h>

#include "ExynosCameraUtils.h"
#include "ExynosCameraProperty.h"
#include "ExynosCameraFrame.h"

#include "ExynosGraphicBuffer.h"

/**
 * Static Variables
 */
#define MAPPER_PARAM(_type, _struct_name, _field, _arraySize) \
    MAKE_STRING(_field), TYPE_ ## _type, offsetof(struct _struct_name, _field), _arraySize

#define ASIGN_NEW_MAPPER(_mapper, _type, _struct_name, _field, _arraySize) \
    MetaMapper * _struct_name ## _ ## _field  = new MetaMapper(MAPPER_PARAM(_type, _struct_name, _field, _arraySize)); \
    (_mapper)->child[MAKE_STRING(_field)] = _struct_name ## _ ## _field

using vendor::graphics::BufferUsage;
using vendor::graphics::ExynosGraphicBufferUsage;
using vendor::graphics::ExynosGraphicBufferMeta;

ExynosCameraProperty ExynosCameraLogManager::m_property;
Mutex ExynosCameraLogManager::m_perfLock[PERFORMANCE_TYPE_MAX];
std::map<int64_t, nsecs_t> ExynosCameraLogManager::m_perfTimestamp[PERFORMANCE_TYPE_MAX][PERFORMANCE_POS_MAX];
std::map<int32_t, ExynosCameraLogManager::TimeInfo> ExynosCameraLogManager::m_perfSummary[PERFORMANCE_TYPE_MAX];
ExynosCameraLogManager::MetaMapper ExynosCameraLogManager::kLv1MetaMap(MAPPER_PARAM(STRUCT, camera2_shot_ext, shot, 0));
std::map<std::string, std::string> ExynosCameraLogManager::kMetaChangeMap;
std::vector<std::string> prev_sV;

/**
 * Internal inline function
 */
inline const char *getPerframeTypeStr(ExynosCameraLogManager::PerframeType type)
{
    switch (type) {
    case ExynosCameraLogManager::PERFRAME_TYPE_SIZE:  return "PFRM_SIZE";
    case ExynosCameraLogManager::PERFRAME_TYPE_PATH:  return "PFRM_PATH";
    case ExynosCameraLogManager::PERFRAME_TYPE_BUF:   return "PFRM_BUF";
    case ExynosCameraLogManager::PERFRAME_TYPE_META:  return "PFRM_META";
    default:                                          return "?";
    }
}

inline const char *getPerformanceTypeStr(ExynosCameraLogManager::PerformanceType type)
{
    switch (type) {
    case ExynosCameraLogManager::PERFORMANCE_TYPE_FPS:  return "PFM_FPS";
    default:                                            return "?";
    }
}

inline const char *getPerfPosStr(ExynosCameraLogManager::PerformancePos pos)
{
    switch (pos) {
    case ExynosCameraLogManager::PERF_FPS_SERVICE_REQUEST:  return "SERV_REQ";
    case ExynosCameraLogManager::PERF_FPS_FRAME_CREATE   :  return "FRM_CRE";
    case ExynosCameraLogManager::PERF_FPS_PATIAL_RESULT  :  return "PTL_RST";
    case ExynosCameraLogManager::PERF_FPS_FRAME_COMPLETE :  return "FRM_COM";
    case ExynosCameraLogManager::PERF_FPS_ALL_RESULT     :  return "ALL_RST";
    default:                                               return "?";
    }
}

inline const char *getFactoryStr(enum FRAME_FACTORY_TYPE factoryType)
{
    switch (factoryType) {
    case FRAME_FACTORY_TYPE_REPROCESSING:       return "reproc";
    case FRAME_FACTORY_TYPE_CAPTURE_PREVIEW:    return "cap_prev";
    case FRAME_FACTORY_TYPE_VISION:             return "vision";
    default:                                    return "?";
    }
}

inline int32_t m_makeSummaryKey(int cameraId, int factoryType,
        ExynosCameraLogManager::TimeCalcType tType, int pos)
{
    return (cameraId << 15) | (factoryType << 10) | (tType << 5) | pos;
}

inline int64_t m_makeTimestampKey(int cameraId, int factoryType, int key)
{
    return (static_cast<int64_t>(cameraId)    << 37) |
           (static_cast<int64_t>(factoryType) << 32) |
           (static_cast<int64_t>(key % 1000)  << 0); // moduler 1000
}

/**
 * Constructor (private)
 */
ExynosCameraLogManager::ExynosCameraLogManager()
{
    /* can't create an instance */
}

/**
 * Public API
 */
void ExynosCameraLogManager::logCommon(android_LogPriority prio,
            const char* tag,
            const char* prefix,
            int cameraId,
            const char* name,
            const char* func,
            int line,
            const char* fmt,
            ...)
{
    status_t ret;
    std::string propValue;
    char level;

    if (prio == ANDROID_LOG_VERBOSE) {
        // 1. persist.log.tag check for verbose

        if (android::getLogEnablePropertyValue() == 0)
            return;

        ret = m_property.get(ExynosCameraProperty::PERSIST_LOG_TAG, tag, propValue);

        if (ret != NO_ERROR) return;
        if (propValue.length() == 0) return;

        level = toupper(propValue.at(0));

        if (level != 'V') {

            // 2. log.tag check for verbose
            ret = m_property.get(ExynosCameraProperty::_LOG_TAG, tag, propValue);

            if (ret != NO_ERROR) return;
            if (propValue.length() == 0) return;

            level = toupper(propValue.at(0));

            if (level != 'V') return;
        }
    }

    // 3. assemble all the string
    String8 logStr;
    ret = logStr.appendFormat(prefix, cameraId, name, func, line);

    va_list args;
    va_start(args, fmt);
    ret = logStr.appendFormatV(fmt, args);
    va_end(args);

    // 4. debugging check
    m_logDebug(prio, tag, logStr);

    // 5. print log
    m_printLog(prio, tag, logStr);
}

void ExynosCameraLogManager::logPerframe(android_LogPriority prio,
            const char* tag,
            const char* prefix,
            int cameraId,
            const char* name,
            const char* func,
            int line,
            const char* fmt,
            PerframeType type,
            void *frame,
            void *data,
            uint32_t requestKey,
            ...)
{
    status_t ret;
    std::string propValue;
    bool enable;

    if (android::getLogEnablePropertyValue() != 1)
        return;

    // 1. global enable check
    ret = m_property.get(ExynosCameraProperty::LOG_PERFRAME_ENABLE, tag, enable);

    if (ret != NO_ERROR) return;
    if (!enable) return;

    // 2. type enable check
    switch (type) {
    case PERFRAME_TYPE_SIZE:
        ret = m_property.get(ExynosCameraProperty::LOG_PERFRAME_SIZE_ENABLE, tag, enable);
        break;
    case PERFRAME_TYPE_PATH:
        ret = m_property.get(ExynosCameraProperty::LOG_PERFRAME_PATH_ENABLE, tag, enable);
        break;
    case PERFRAME_TYPE_BUF:
        ret = m_property.get(ExynosCameraProperty::LOG_PERFRAME_BUF_ENABLE, tag, enable);
        break;
    case PERFRAME_TYPE_META:
        ret = m_property.get(ExynosCameraProperty::LOG_PERFRAME_META_ENABLE, tag, enable);
        break;
    default:
        return;
    }

    if (ret != NO_ERROR) return;
    if (!enable) return;

    // 3. cameraId check
    if (cameraId >= 0) {
        std::vector<int32_t> i32V;

        ret = m_property.getVector(ExynosCameraProperty::LOG_PERFRAME_FILTER_CAMID, tag, i32V);

        if (ret != NO_ERROR) return;

        if (i32V.size() > 0) {
            enable = false;

            for (int32_t each : i32V) {
                if (cameraId == each) {
                    enable = true;
                    break;
                }
            }

            if (!enable) return;
        }
    }

    ExynosCameraFrame *framePtr = static_cast<ExynosCameraFrame *>(frame);

    // 4. factory check
    if (framePtr != nullptr) {
        std::vector<std::string> sV;

        ret = m_property.getVector(ExynosCameraProperty::LOG_PERFRAME_FILTER_FACTORY, tag, sV);

        if (ret != NO_ERROR) return;

        if (sV.size() > 0) {
            enable = false;
            const char* curFactoryType = getFactoryStr(framePtr->getFactoryType());

            for (std::string each : sV) {
                if (each.compare(curFactoryType) == 0) {
                    enable = true;
                    break;
                }
            }

            if (!enable) return;
        }
    }

    // 5. assemble all the string
    String8 logStr;
    if (framePtr != nullptr) {
        ret = logStr.appendFormat(prefix, cameraId, name, func, line,
                framePtr->getFactoryType(), framePtr->getFrameCount(),
                framePtr->getCameraId(), framePtr->getMetaFrameCount(),
                framePtr->getFrameType(), framePtr->getFrameState(),
                framePtr->getRequestKey());
    } else {
        ret = logStr.appendFormat(prefix, cameraId, name, func, line,
                -1, -1, -1, -1, -1, -1, requestKey);
    }

    logStr.appendFormat("[%s] ", getPerframeTypeStr(type));

    // 6. additional info by perframe type
    switch (type) {
    case PERFRAME_TYPE_PATH:
        if (framePtr != nullptr)
            framePtr->getEntityInfoStr(logStr);
        break;
    case PERFRAME_TYPE_META:
        break;
    case PERFRAME_TYPE_BUF:
        {
            camera3_stream_buffer *pStreamBuf = static_cast<camera3_stream_buffer *>(data);
            if (pStreamBuf != nullptr) {
                vendor::graphics::ExynosGraphicBufferMeta gmeta(*pStreamBuf->buffer);
                logStr.appendFormat("INFO/ adr:%p,size:%dx%d,format:%x,usage:%x,fence:%d/%d,sts:%d,buf(%p),hdl:%p(%p) "
                                    "numFd:%d,numInt:%d,fd:%d/%d/%d,usage:(P%jx/C%jx),fmt:(I%jx/F%x),size:%d/%d/%d ",
                                    pStreamBuf,
                                    pStreamBuf->stream->width, pStreamBuf->stream->height,
                                    pStreamBuf->stream->format, pStreamBuf->stream->usage,
                                    pStreamBuf->acquire_fence, pStreamBuf->release_fence, pStreamBuf->status, pStreamBuf->buffer,
                                    *(pStreamBuf->buffer), *pStreamBuf->buffer,
                                    (*(pStreamBuf->buffer))->numFds, (*(pStreamBuf->buffer))->numInts,
                                    gmeta.fd, gmeta.fd1, gmeta.fd2,
                                    gmeta.producer_usage, gmeta.consumer_usage,
                                    gmeta.internal_format, gmeta.frameworkFormat,
                                    gmeta.size, gmeta.size1, gmeta.size2);
            }
        }
        break;
    default:
        break;
    }

    va_list args;
    va_start(args, requestKey);
    ret = logStr.appendFormatV(fmt, args);
    va_end(args);

    // 6. check some kind of filter
    switch (type) {
    case PERFRAME_TYPE_BUF:
        {
            // Don't filtering except for BufferManager
            std::string filterClass = "ExynosCameraBufferManager";
            if (filterClass.compare(tag) != 0) break;

            std::vector<std::string> sV;

            ret = m_property.getVector(ExynosCameraProperty::LOG_PERFRAME_BUF_FILTER, tag, sV);

            if (ret != NO_ERROR) return;

            if (sV.size() > 0) {
                enable = false;

                for (std::string each : sV) {
                    if (each.compare(name) == 0) enable = true;
                }

                if (!enable) return;
            }
        }
        break;
    case PERFRAME_TYPE_META:
        {
            String8 valueStr;
            String8 tmplog;
            string initstr = "";
            bool flagChangePrint = true;
            std::vector<std::string> sV;
            std::vector<std::string> sUserValueV;

            // filter
            ret = m_property.getVector(ExynosCameraProperty::LOG_PERFRAME_META_FILTER, tag, sV);
            if (ret != NO_ERROR) return;

            if (sV != prev_sV) {
                prev_sV = sV;
                m_property.set(ExynosCameraProperty::LOG_PERFRAME_META_VALUES,tag,initstr.c_str());
            }

            // value
            ret = m_property.getVector(ExynosCameraProperty::LOG_PERFRAME_META_VALUES, tag, sUserValueV);
            if (ret != NO_ERROR) return;

            // change print
            ret = m_property.get(ExynosCameraProperty::LOG_PERFRAME_META_CHANGE_PRINT, tag, flagChangePrint);
            if (ret != NO_ERROR) return;

            enable = false;

            bool userValueSet = false;
            std::vector<std::string>::iterator valueIter = sUserValueV.begin();
            if (sV.size() == sUserValueV.size()) userValueSet = true;

            if (sV.size() > 0) {
                const std::regex re("[.]+"); // delimeter is '.'
                tmplog="";
                for (std::string each : sV) {
                    std::sregex_token_iterator it(each.begin(), each.end(), re, -1);
                    std::sregex_token_iterator reg_end;

                    long offset = 0;
                    valueStr="";
                    MetaMapper *mapper = &kLv1MetaMap;
                    for (; it != reg_end; ++it) {
                        if (it->str().length() > 0) {
                            mapper = mapper->child[it->str().c_str()];
                            if (mapper != nullptr) {
                                // find final fiels's address offset
                                offset += mapper->offset;
                            } else {
                                // nothing to find
                                return;
                            }
                        } else {
                            break;
                        }
                    }

                    // find!!
                    if (mapper->type != TYPE_STRUCT) {
                        enable = true;

                        logStr.appendFormat("{%s(%d):", each.c_str(), mapper->type);
                        uint8_t *adr = static_cast<uint8_t *>(data) + offset;

                        /**
                         * user value set (list)
                         * eg. filter : ctl.aa.afReions|ctl.aa.aeTargetFpsRange|ctl.aa.ae.awbMode
                         *     value  : 0,10,10,300,300|30,30|3
                         *     =>       afReions[0] = 0, [1] = 10, [2] = 10, [3] = 300, [4] = 300
                         *              aeTargetFpsRange[0] = 30, aeTargetFpsRange[1] = 30
                         *              awbMode = 3
                         */
                        std::list<std::string> splitL;
                        if (userValueSet) {
                            const std::regex valueRe("[,]+"); // delimeter is ','
                            std::sregex_token_iterator reg_value_iter(valueIter->begin(),
                                    valueIter->end(), valueRe, -1);
                            std::sregex_token_iterator reg_value_end;
                            ++valueIter;

                            for (; reg_value_iter != reg_value_end; ++reg_value_iter) {
                                splitL.push_back(reg_value_iter->str());
                            }
                        }

                        bool isValidUserValueSet = false;
                        bool isHexaModeSet = false;
                        std::string mapper_name(mapper->str);
                        if (splitL.size() == mapper->arraySize) isValidUserValueSet = true;
                        if (splitL.front() == "x" && splitL.size() == 3) {
                            isValidUserValueSet = true;
                            isHexaModeSet = true;
                            splitL.pop_front();
                        }
                        if(splitL.front() == " ") isValidUserValueSet=false;

                        if (splitL.front() == " ") {
                            isValidUserValueSet=false;
                        }

                        for (uint32_t i = 0; i < mapper->arraySize; i++) {
                            switch (mapper->type) {
                            case TYPE_INT8:
                                valueStr.appendFormat("[%d]:%d,", i, *((uint8_t *)adr));
                                if (isValidUserValueSet) {
                                    int value = std::stoi(splitL.front());
                                    splitL.pop_front();
                                    *((uint8_t *)adr) = (uint8_t)value;
                                }
                                adr += sizeof(uint8_t);
                                break;
                            case TYPE_INT16:
                                valueStr.appendFormat("[%d]:%d,", i, *((uint16_t *)adr));
                                if (isValidUserValueSet) {
                                    int value = std::stoi(splitL.front());
                                    splitL.pop_front();
                                    *((uint16_t *)adr) = (uint16_t)value;
                                }
                                adr += sizeof(uint16_t);
                                break;
                            case TYPE_INT32:
                                valueStr.appendFormat("[%d]:%d,", i, *((uint32_t *)adr));
                                if (isValidUserValueSet) {
                                    uint32_t value = std::stoi(splitL.front());
                                    splitL.pop_front();
                                    if (isHexaModeSet) {
                                        value = value << 16;
                                        uint32_t tmpvalue = std::stoi(splitL.front());
                                        splitL.pop_front();
                                        value |= tmpvalue;
                                    }
                                    *((uint32_t *)adr) = value;
                                }
                                adr += sizeof(uint32_t);
                                break;
                            case TYPE_INT64:
                                valueStr.appendFormat("[%d]:%ju,", i, *((uint64_t *)adr));
                                if (isValidUserValueSet) {
                                    uint64_t value = std::stoll(splitL.front());
                                    splitL.pop_front();
                                    *((uint64_t *)adr) = value;
                                }
                                adr += sizeof(uint64_t);
                                break;
                            case TYPE_FLOAT:
                                valueStr.appendFormat("[%d]:%f,", i, *((float *)adr));
                                if (isValidUserValueSet) {
                                    float value = std::stof(splitL.front());
                                    splitL.pop_front();
                                    *((float *)adr) = value;
                                }
                                adr += sizeof(float);
                                break;
                            case TYPE_DOUBLE:
                                valueStr.appendFormat("[%d]:%f,", i, *((double *)adr));
                                if (isValidUserValueSet) {
                                    double value = std::stod(splitL.front());
                                    splitL.pop_front();
                                    *((double *)adr) = value;
                                }
                                adr += sizeof(double);
                                break;
                            case TYPE_RATIONAL:
                                {
                                    struct rational *r = (struct rational *)adr;
                                    valueStr.appendFormat("[%d]:%d/%d,", i, r->num, r->den);
                                    if (isValidUserValueSet) {
                                        uint64_t value = std::stoll(splitL.front());
                                        splitL.pop_front();
                                        r->num = (uint32_t)(value >> 32);
                                        r->den = (uint32_t)(value & 0xFFFFFFFFLL);
                                    }
                                    adr += sizeof(struct rational);
                                }
                                break;
                            default:
                                break;
                            }
                        }

                        logStr.append(valueStr);
                        tmplog.append(valueStr);
                        logStr.append("},");
                    }

                }
                // print if the meta's value was changed
                if (flagChangePrint) {
                // key to trace only changed value
                    std::string key;
                    int i=0;
                    key.append(tmplog);
                    key.append(std::to_string(line));
                    key.append(name);
                    std::string prevValue = kMetaChangeMap[key];
                    kMetaChangeMap[key] = tmplog.string();
                    if (prevValue.compare(tmplog.string()) == 0)
                        return;
                } else {
                        kMetaChangeMap.clear();
                }
            }
            if (!enable) return;
        }
        break;
    case PERFRAME_TYPE_SIZE:
    case PERFRAME_TYPE_PATH:
    case PERFRAME_TYPE_MAX:
        /* nothing to do */
        break;
    }

    // 7. debugging check
    m_logDebug(prio, tag, logStr);

    // 8. print log
    m_printLog(prio, tag, logStr);
}

void ExynosCameraLogManager::init(void) {
    m_initMetaMapper();

    // performance init
    for (int i = 0; i < PERFORMANCE_TYPE_MAX; i++) {
        m_perfSummary[i].clear();
        for (int j = 0; j < PERFORMANCE_POS_MAX; j++) {
            m_perfTimestamp[i][j].clear();
        }
    }
}

void ExynosCameraLogManager::logPerformance(android_LogPriority prio,
            const char* tag,
            const char* prefix,
            int cameraId,
            int factoryType,
            PerformanceType pType,
            TimeCalcType tType,
            int pos,
            __unused int posSubKey,  /* eg.pipeId */
            int key,        /* eg.requestKey */
            void *data      /* eg.ExynosCameraFrame */
            ...)
{
    status_t ret;
    std::string propValue;
    bool enable;

    // 1. global enable check
    ret = m_property.get(ExynosCameraProperty::LOG_PERFORMANCE_ENABLE, tag, enable);

    if (ret != NO_ERROR) return;
    if (!enable) return;

    // 2. type enable check
    switch (pType) {
    case ExynosCameraLogManager::PERFORMANCE_TYPE_FPS:
        ret = m_property.get(ExynosCameraProperty::LOG_PERFORMANCE_FPS_ENABLE, tag, enable);
        break;
    default:
        return;
    }

    if (ret != NO_ERROR) return;
    if (!enable) return;

    switch (pType) {
    case PERFORMANCE_TYPE_FPS:
        m_logPerformanceFps(prio, tag, prefix, cameraId, factoryType,
                pType, tType, pos, key, data);
        break;
    default:
        break;
    }
}

/**
 * Not Public Functions (Internal)
 */
void ExynosCameraLogManager::m_initMetaMapper(void) {
    kMetaChangeMap.clear();

    if (kLv1MetaMap.child.size() > 0) return;

    // root : ctl, dm (uctl, udm not supported)
    ASIGN_NEW_MAPPER(&kLv1MetaMap, STRUCT, camera2_shot, ctl,  1);
    ASIGN_NEW_MAPPER(&kLv1MetaMap, STRUCT, camera2_shot, dm ,  1);
    ASIGN_NEW_MAPPER(&kLv1MetaMap, STRUCT, camera2_shot, uctl, 1);
    ASIGN_NEW_MAPPER(&kLv1MetaMap, STRUCT, camera2_shot, udm,  1);

    // ctl
    ASIGN_NEW_MAPPER(camera2_shot_ctl, STRUCT, camera2_ctl, color     , 1);
    ASIGN_NEW_MAPPER(camera2_shot_ctl, STRUCT, camera2_ctl, aa        , 1);
    ASIGN_NEW_MAPPER(camera2_shot_ctl, STRUCT, camera2_ctl, demosaic  , 1);
    ASIGN_NEW_MAPPER(camera2_shot_ctl, STRUCT, camera2_ctl, edge      , 1);
    ASIGN_NEW_MAPPER(camera2_shot_ctl, STRUCT, camera2_ctl, flash     , 1);
    ASIGN_NEW_MAPPER(camera2_shot_ctl, STRUCT, camera2_ctl, hotpixel  , 1);
    ASIGN_NEW_MAPPER(camera2_shot_ctl, STRUCT, camera2_ctl, jpeg      , 1);
    ASIGN_NEW_MAPPER(camera2_shot_ctl, STRUCT, camera2_ctl, lens      , 1);
    ASIGN_NEW_MAPPER(camera2_shot_ctl, STRUCT, camera2_ctl, noise     , 1);
    ASIGN_NEW_MAPPER(camera2_shot_ctl, STRUCT, camera2_ctl, request   , 1);
    ASIGN_NEW_MAPPER(camera2_shot_ctl, STRUCT, camera2_ctl, scaler    , 1);
    ASIGN_NEW_MAPPER(camera2_shot_ctl, STRUCT, camera2_ctl, sensor    , 1);
    ASIGN_NEW_MAPPER(camera2_shot_ctl, STRUCT, camera2_ctl, shading   , 1);
    ASIGN_NEW_MAPPER(camera2_shot_ctl, STRUCT, camera2_ctl, stats     , 1);
    ASIGN_NEW_MAPPER(camera2_shot_ctl, STRUCT, camera2_ctl, tonemap   , 1);
    ASIGN_NEW_MAPPER(camera2_shot_ctl, STRUCT, camera2_ctl, led       , 1);
    ASIGN_NEW_MAPPER(camera2_shot_ctl, STRUCT, camera2_ctl, blacklevel, 1);
    ASIGN_NEW_MAPPER(camera2_shot_ctl, STRUCT, camera2_ctl, sync      , 1);
    ASIGN_NEW_MAPPER(camera2_shot_ctl, STRUCT, camera2_ctl, reprocess , 1);

    //clt, vendor_entry
    ASIGN_NEW_MAPPER(camera2_shot_ctl, STRUCT, camera2_ctl, vendor_entry , 1);

    //ctl, color
    ASIGN_NEW_MAPPER(camera2_ctl_color, INT32,    camera2_colorcorrection_ctl, mode,         1);
    ASIGN_NEW_MAPPER(camera2_ctl_color, RATIONAL, camera2_colorcorrection_ctl, transform,    9);
    ASIGN_NEW_MAPPER(camera2_ctl_color, FLOAT,    camera2_colorcorrection_ctl, gains,        4);
    ASIGN_NEW_MAPPER(camera2_ctl_color, INT32,    camera2_colorcorrection_ctl, aberrationCorrectionMode, 1);
    ASIGN_NEW_MAPPER(camera2_ctl_color, INT32,    camera2_colorcorrection_ctl, vendor_hue,   1);
    ASIGN_NEW_MAPPER(camera2_ctl_color, INT32,    camera2_colorcorrection_ctl, vendor_saturation, 1);
    ASIGN_NEW_MAPPER(camera2_ctl_color, INT32,    camera2_colorcorrection_ctl, vendor_brightness, 1);
    ASIGN_NEW_MAPPER(camera2_ctl_color, INT32,    camera2_colorcorrection_ctl, vendor_contrast,  1);

    //ctl, aa
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, aeAntibandingMode           ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, aeExpCompensation           ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, aeLock                      ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, aeMode                      ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, aeRegions                   ,     5);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, aeTargetFpsRange            ,     2);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, aePrecaptureTrigger         ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, afMode                      ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, afRegions                   ,     5);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, afTrigger                   ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, awbLock                     ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, awbMode                     ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, awbRegions                  ,     5);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, captureIntent               ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, effectMode                  ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, mode                        ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, sceneMode                   ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, videoStabilizationMode      ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, FLOAT, camera2_aa_ctl, vendor_aeExpCompensationStep,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, vendor_afmode_option        ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, vendor_afmode_ext           ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, vendor_aeflashMode          ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, vendor_isoMode              ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, vendor_isoValue             ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, vendor_awbValue             ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, vendor_cameraId             ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, vendor_videoMode            ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, vendor_aeFaceMode           ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, vendor_afState              ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, vendor_exposureValue        ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, vendor_touchAeDone          ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, vendor_touchBvChange        ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, vendor_captureCount         ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, vendor_captureExposureTime  ,     1);
#if defined(USE_CAMERA_EXYNOS9630_META) || defined(USE_CAMERA_EXYNOS850_META) || defined(USE_CAMERA_EXYNOS3830_META)
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, vendor_objectDistanceCm     ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, vendor_colorTempKelvin      ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, vendor_enableDynamicShotDm  ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, FLOAT, camera2_aa_ctl, vendor_expBracketing        ,    15);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, FLOAT, camera2_aa_ctl, vendor_expBracketingCapture ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, vendor_enableSuperNight     ,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_aa, INT32, camera2_aa_ctl, vendor_reserved             ,     7);
#endif

    //ctl, demosaic
    ASIGN_NEW_MAPPER(camera2_ctl_demosaic, INT32, camera2_demosaic_ctl, mode, 1);

    //ctl, edge
    ASIGN_NEW_MAPPER(camera2_ctl_edge, INT32, camera2_edge_ctl, mode    , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_edge, INT8,  camera2_edge_ctl, strength, 1);

    //ctl, flash
    ASIGN_NEW_MAPPER(camera2_ctl_flash, INT32, camera2_flash_ctl, firingPower, 1);
    ASIGN_NEW_MAPPER(camera2_ctl_flash, INT64, camera2_flash_ctl, firingTime , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_flash, INT32, camera2_flash_ctl, flashMode  , 1);

    //ctl, hotpixel
    ASIGN_NEW_MAPPER(camera2_ctl_hotpixel, INT32, camera2_hotpixel_ctl, mode, 1);

    //ctl, jpeg
    ASIGN_NEW_MAPPER(camera2_ctl_jpeg, INT8  , camera2_jpeg_ctl, gpsLocation        , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_jpeg, DOUBLE, camera2_jpeg_ctl, gpsCoordinates     , 3);
    ASIGN_NEW_MAPPER(camera2_ctl_jpeg, INT8  , camera2_jpeg_ctl, gpsProcessingMethod, 33);
    ASIGN_NEW_MAPPER(camera2_ctl_jpeg, INT64 , camera2_jpeg_ctl, gpsTimestamp       , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_jpeg, INT32 , camera2_jpeg_ctl, orientation        , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_jpeg, INT8  , camera2_jpeg_ctl, quality            , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_jpeg, INT8  , camera2_jpeg_ctl, thumbnailQuality   , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_jpeg, INT32 , camera2_jpeg_ctl, thumbnailSize      , 2);

    //ctl, lens
    ASIGN_NEW_MAPPER(camera2_ctl_lens, INT32, camera2_lens_ctl, aperture     , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_lens, FLOAT, camera2_lens_ctl, filterDensity , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_lens, FLOAT, camera2_lens_ctl, focalLength   , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_lens, FLOAT, camera2_lens_ctl, focusDistance , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_lens, INT32, camera2_lens_ctl, opticalStabilizationMode, 1);

    //ctl, noisereduction
    ASIGN_NEW_MAPPER(camera2_ctl_noise, INT32, camera2_noisereduction_ctl, mode,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_noise, INT8 , camera2_noisereduction_ctl, strength, 1);

    //ctl, request
    ASIGN_NEW_MAPPER(camera2_ctl_request, INT32, camera2_request_ctl, frameCount,       1);
    ASIGN_NEW_MAPPER(camera2_ctl_request, INT32, camera2_request_ctl, id,               1);
    ASIGN_NEW_MAPPER(camera2_ctl_request, INT32, camera2_request_ctl, metadataMode,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_request, INT32, camera2_request_ctl, vendor_frameCount,1);

    //ctl, scaler
    ASIGN_NEW_MAPPER(camera2_ctl_scaler, INT32, camera2_scaler_ctl, cropRegion, 4);

    //ctl, sensor
    ASIGN_NEW_MAPPER(camera2_ctl_sensor, INT64, camera2_sensor_ctl, exposureTime   , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_sensor, INT64, camera2_sensor_ctl, frameDuration  , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_sensor, INT32, camera2_sensor_ctl, sensitivity    , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_sensor, INT32, camera2_sensor_ctl, testPatternData, 4);
    ASIGN_NEW_MAPPER(camera2_ctl_sensor, INT32, camera2_sensor_ctl, testPatternMode, 1);

    //ctl, shading
    ASIGN_NEW_MAPPER(camera2_ctl_shading, INT32, camera2_shading_ctl, mode,     1);
    ASIGN_NEW_MAPPER(camera2_ctl_shading, INT8 , camera2_shading_ctl, strength, 1);

    //ctl, stats
    ASIGN_NEW_MAPPER(camera2_ctl_stats, INT32, camera2_stats_ctl, faceDetectMode    , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_stats, INT32, camera2_stats_ctl, histogramMode     , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_stats, INT32, camera2_stats_ctl, sharpnessMapMode  , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_stats, INT32, camera2_stats_ctl, hotPixelMapMode   , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_stats, INT32, camera2_stats_ctl, lensShadingMapMode, 1);

    //ctl, tonemap
    ASIGN_NEW_MAPPER(camera2_ctl_tonemap, FLOAT, camera2_tonemap_ctl, curveBlue , 64);
    ASIGN_NEW_MAPPER(camera2_ctl_tonemap, FLOAT, camera2_tonemap_ctl, curveGreen, 64);
    ASIGN_NEW_MAPPER(camera2_ctl_tonemap, FLOAT, camera2_tonemap_ctl, curveRed  , 64);
    ASIGN_NEW_MAPPER(camera2_ctl_tonemap, FLOAT, camera2_tonemap_ctl, curve     , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_tonemap, INT32, camera2_tonemap_ctl, mode      , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_tonemap, FLOAT, camera2_tonemap_ctl, gamma     , 1);
    ASIGN_NEW_MAPPER(camera2_ctl_tonemap, INT32, camera2_tonemap_ctl, presetCurve, 1);

    //ctl, led
    ASIGN_NEW_MAPPER(camera2_ctl_led, INT32, camera2_led_ctl, transmit, 1);

    //ctl, blacklevel
    ASIGN_NEW_MAPPER(camera2_ctl_blacklevel, INT32, camera2_blacklevel_ctl, lock, 1);

    //ctl, sync
    ASIGN_NEW_MAPPER(camera2_ctl_sync, INT64, camera2_sync_ctl, frameNumber, 1);

    //ctl, reprocess
    ASIGN_NEW_MAPPER(camera2_ctl_reprocess, FLOAT, camera2_reprocess_ctl, effectiveExposureFactor, 1);

    //ctl, entry
    ASIGN_NEW_MAPPER(camera2_ctl_vendor_entry, INT32, camera2_entry_ctl, lowIndexParam    ,    1);
    ASIGN_NEW_MAPPER(camera2_ctl_vendor_entry, INT32, camera2_entry_ctl, highIndexParam   ,    1);
    ASIGN_NEW_MAPPER(camera2_ctl_vendor_entry, INT32, camera2_entry_ctl, parameter        , 2048);

    // dm
    ASIGN_NEW_MAPPER(camera2_shot_dm, STRUCT, camera2_dm, color     , 1);
    ASIGN_NEW_MAPPER(camera2_shot_dm, STRUCT, camera2_dm, aa        , 1);
    ASIGN_NEW_MAPPER(camera2_shot_dm, STRUCT, camera2_dm, demosaic  , 1);
    ASIGN_NEW_MAPPER(camera2_shot_dm, STRUCT, camera2_dm, edge      , 1);
    ASIGN_NEW_MAPPER(camera2_shot_dm, STRUCT, camera2_dm, flash     , 1);
    ASIGN_NEW_MAPPER(camera2_shot_dm, STRUCT, camera2_dm, hotpixel  , 1);
    ASIGN_NEW_MAPPER(camera2_shot_dm, STRUCT, camera2_dm, jpeg      , 1);
    ASIGN_NEW_MAPPER(camera2_shot_dm, STRUCT, camera2_dm, lens      , 1);
    ASIGN_NEW_MAPPER(camera2_shot_dm, STRUCT, camera2_dm, noise     , 1);
    ASIGN_NEW_MAPPER(camera2_shot_dm, STRUCT, camera2_dm, request   , 1);
    ASIGN_NEW_MAPPER(camera2_shot_dm, STRUCT, camera2_dm, scaler    , 1);
    ASIGN_NEW_MAPPER(camera2_shot_dm, STRUCT, camera2_dm, sensor    , 1);
    ASIGN_NEW_MAPPER(camera2_shot_dm, STRUCT, camera2_dm, shading   , 1);
    ASIGN_NEW_MAPPER(camera2_shot_dm, STRUCT, camera2_dm, stats     , 1);
    ASIGN_NEW_MAPPER(camera2_shot_dm, STRUCT, camera2_dm, tonemap   , 1);
    ASIGN_NEW_MAPPER(camera2_shot_dm, STRUCT, camera2_dm, led       , 1);
    ASIGN_NEW_MAPPER(camera2_shot_dm, STRUCT, camera2_dm, blacklevel, 1);
    ASIGN_NEW_MAPPER(camera2_shot_dm, STRUCT, camera2_dm, sync      , 1);
    ASIGN_NEW_MAPPER(camera2_shot_dm, STRUCT, camera2_dm, reprocess , 1);

    //dm ,vendor
    ASIGN_NEW_MAPPER(camera2_shot_dm, STRUCT, camera2_dm, vendor_entry , 1);

    //dm, color
    ASIGN_NEW_MAPPER(camera2_dm_color, INT32,    camera2_colorcorrection_dm, mode,         1);
    ASIGN_NEW_MAPPER(camera2_dm_color, RATIONAL, camera2_colorcorrection_dm, transform,    9);
    ASIGN_NEW_MAPPER(camera2_dm_color, FLOAT,    camera2_colorcorrection_dm, gains,        4);
    ASIGN_NEW_MAPPER(camera2_dm_color, INT32,    camera2_colorcorrection_dm, aberrationCorrectionMode, 1);
    ASIGN_NEW_MAPPER(camera2_dm_color, INT32,    camera2_colorcorrection_dm, vendor_hue,   1);
    ASIGN_NEW_MAPPER(camera2_dm_color, INT32,    camera2_colorcorrection_dm, vendor_saturation, 1);
    ASIGN_NEW_MAPPER(camera2_dm_color, INT32,    camera2_colorcorrection_dm, vendor_brightness, 1);
    ASIGN_NEW_MAPPER(camera2_dm_color, INT32,    camera2_colorcorrection_dm, vendor_contrast,  1);

    //dm, aa
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, aeAntibandingMode           ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, aeExpCompensation           ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, aeLock                      ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, aeMode                      ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, aeRegions                   ,     5);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, aeTargetFpsRange            ,     2);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, aePrecaptureTrigger         ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, afMode                      ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, afRegions                   ,     5);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, afTrigger                   ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, afState                     ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, awbLock                     ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, awbMode                     ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, awbRegions                  ,     5);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, captureIntent               ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, awbState                    ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, effectMode                  ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, mode                        ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, sceneMode                   ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, videoStabilizationMode      ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, FLOAT, camera2_aa_dm, vendor_aeExpCompensationStep,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_afmode_option        ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_afmode_ext           ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_aeflashMode          ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_isoMode              ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_isoValue             ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_awbValue             ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_cameraId             ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_videoMode            ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_aeFaceMode           ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_afState              ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_exposureValue        ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_touchAeDone          ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_touchBvChange        ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_captureCount         ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_captureExposureTime  ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, FLOAT, camera2_aa_dm, vendor_objectDistanceCm     ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_colorTempKelvin      ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, FLOAT, camera2_aa_dm, vendor_expBracketing        ,    15);
    ASIGN_NEW_MAPPER(camera2_dm_aa, FLOAT, camera2_aa_dm, vendor_expBracketingCapture ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_dynamicShotValue     ,     3);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_lightConditionValue  ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_dynamicShotExtraInfo ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, STRUCT, camera2_aa_dm, vendor_apexInfo            ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, STRUCT, camera2_aa_dm, vendor_osdInfo             ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_drcRatio             ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_colorTempIndex       ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_luxIndex             ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, FLOAT, camera2_aa_dm, vendor_luxStandard          ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_multiFrameEv         ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_faceToneWeight       ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, FLOAT, camera2_aa_dm, vendor_noiseIndex           ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_aeModeState          ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_linearExposureIndex  ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, ispDigitalGain              ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_previewSkipFrame     ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_reserved             ,    60);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_wideTeleConvEv       ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_teleSync             ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_fusionCaptureAeInfo  ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_fusionCaptureAfInfo  ,     1);
    ASIGN_NEW_MAPPER(camera2_dm_aa, INT32, camera2_aa_dm, vendor_reserved_dual        ,     9);

    //dm,aa,apexInfo
    ASIGN_NEW_MAPPER(camera2_aa_dm_vendor_apexInfo, INT32, aa_apexInfo, av  ,     1);
    ASIGN_NEW_MAPPER(camera2_aa_dm_vendor_apexInfo, INT32, aa_apexInfo, sv  ,     1);
    ASIGN_NEW_MAPPER(camera2_aa_dm_vendor_apexInfo, INT32, aa_apexInfo, tv  ,     1);
    ASIGN_NEW_MAPPER(camera2_aa_dm_vendor_apexInfo, INT32, aa_apexInfo, ev  ,     1);
    ASIGN_NEW_MAPPER(camera2_aa_dm_vendor_apexInfo, INT32, aa_apexInfo, bv  ,     1);

    //dm,aa,osdInfo
    ASIGN_NEW_MAPPER(camera2_aa_dm_vendor_osdInfo, STRUCT , osdInfo , sensorInfo ,     1);
    ASIGN_NEW_MAPPER(camera2_aa_dm_vendor_osdInfo, INT32, osdInfo , reserved    ,     10);

    //dm,aa,osdinfo,sensorInfo
    ASIGN_NEW_MAPPER(camera2_aa_dm_vendor_osdInfo, INT32 ,sensorInfo , analogGain,       1);
    ASIGN_NEW_MAPPER(camera2_aa_dm_vendor_osdInfo, INT32 ,sensorInfo , digitalGain,      1);
    ASIGN_NEW_MAPPER(camera2_aa_dm_vendor_osdInfo, INT64 ,sensorInfo , longExposureTime, 1);
    ASIGN_NEW_MAPPER(camera2_aa_dm_vendor_osdInfo, INT64 ,sensorInfo , midExposureTime,  1);
    ASIGN_NEW_MAPPER(camera2_aa_dm_vendor_osdInfo, INT64 ,sensorInfo , shortExposureTime,1);
    ASIGN_NEW_MAPPER(camera2_aa_dm_vendor_osdInfo, INT32 ,sensorInfo , longAnalogGain,   1);
    ASIGN_NEW_MAPPER(camera2_aa_dm_vendor_osdInfo, INT32 ,sensorInfo , midAnalogGain,    1);
    ASIGN_NEW_MAPPER(camera2_aa_dm_vendor_osdInfo, INT32 ,sensorInfo , shortAnalogGain,  1);
    ASIGN_NEW_MAPPER(camera2_aa_dm_vendor_osdInfo, INT32 ,sensorInfo , longDigitalGain,  1);
    ASIGN_NEW_MAPPER(camera2_aa_dm_vendor_osdInfo, INT32 ,sensorInfo , midDigitalGain,   1);
    ASIGN_NEW_MAPPER(camera2_aa_dm_vendor_osdInfo, INT32 ,sensorInfo , shortDigitalGain, 1);

    //dm, demosaic
    ASIGN_NEW_MAPPER(camera2_dm_demosaic, INT32, camera2_demosaic_dm, mode, 1);

    //dm, edge
    ASIGN_NEW_MAPPER(camera2_dm_edge, INT32, camera2_edge_dm, mode    , 1);
    ASIGN_NEW_MAPPER(camera2_dm_edge, INT8,  camera2_edge_dm, strength, 1);

    //dm, flash
    ASIGN_NEW_MAPPER(camera2_dm_flash, INT32, camera2_flash_dm, firingPower, 1);
    ASIGN_NEW_MAPPER(camera2_dm_flash, INT64, camera2_flash_dm, firingTime , 1);
    ASIGN_NEW_MAPPER(camera2_dm_flash, INT32, camera2_flash_dm, flashMode  , 1);
    ASIGN_NEW_MAPPER(camera2_dm_flash, INT32, camera2_flash_dm, flashState , 1);
    ASIGN_NEW_MAPPER(camera2_dm_flash, INT32, camera2_flash_dm, vendor_firingStable  , 1);
    ASIGN_NEW_MAPPER(camera2_dm_flash, INT32, camera2_flash_dm, vendor_decision      , 1);
    ASIGN_NEW_MAPPER(camera2_dm_flash, INT32, camera2_flash_dm, vendor_flashReady    , 1);
    ASIGN_NEW_MAPPER(camera2_dm_flash, INT32, camera2_flash_dm, vendor_flashOffReady , 1);

    //dm, hotpixel
    ASIGN_NEW_MAPPER(camera2_dm_hotpixel, INT32, camera2_hotpixel_dm, mode, 1);

    //dm, jpeg
    ASIGN_NEW_MAPPER(camera2_dm_jpeg, INT8  , camera2_jpeg_dm, gpsLocation        , 1);
    ASIGN_NEW_MAPPER(camera2_dm_jpeg, DOUBLE, camera2_jpeg_dm, gpsCoordinates     , 3);
    ASIGN_NEW_MAPPER(camera2_dm_jpeg, INT8  , camera2_jpeg_dm, gpsProcessingMethod, 33);
    ASIGN_NEW_MAPPER(camera2_dm_jpeg, INT64 , camera2_jpeg_dm, gpsTimestamp       , 1);
    ASIGN_NEW_MAPPER(camera2_dm_jpeg, INT32 , camera2_jpeg_dm, orientation        , 1);
    ASIGN_NEW_MAPPER(camera2_dm_jpeg, INT8  , camera2_jpeg_dm, quality            , 1);
    ASIGN_NEW_MAPPER(camera2_dm_jpeg, INT32 , camera2_jpeg_dm, size               , 1);
    ASIGN_NEW_MAPPER(camera2_dm_jpeg, INT8  , camera2_jpeg_dm, thumbnailQuality   , 1);
    ASIGN_NEW_MAPPER(camera2_dm_jpeg, INT32 , camera2_jpeg_dm, thumbnailSize      , 2);

    //dm, lens
    ASIGN_NEW_MAPPER(camera2_dm_lens, INT32, camera2_lens_dm, aperture     , 1);
    ASIGN_NEW_MAPPER(camera2_dm_lens, FLOAT, camera2_lens_dm, filterDensity , 1);
    ASIGN_NEW_MAPPER(camera2_dm_lens, FLOAT, camera2_lens_dm, focalLength   , 1);
    ASIGN_NEW_MAPPER(camera2_dm_lens, FLOAT, camera2_lens_dm, focusDistance , 1);
    ASIGN_NEW_MAPPER(camera2_dm_lens, FLOAT, camera2_lens_dm, focusRange    , 2);
    ASIGN_NEW_MAPPER(camera2_dm_lens, INT32, camera2_lens_dm, opticalStabilizationMode, 1);
    ASIGN_NEW_MAPPER(camera2_dm_lens, INT32, camera2_lens_dm, state         , 1);
    ASIGN_NEW_MAPPER(camera2_dm_lens, FLOAT, camera2_lens_dm, poseRotation  , 4);
    ASIGN_NEW_MAPPER(camera2_dm_lens, FLOAT, camera2_lens_dm, poseTranslation, 3);
    ASIGN_NEW_MAPPER(camera2_dm_lens, FLOAT, camera2_lens_dm, intrinsicCalibration, 5);
    ASIGN_NEW_MAPPER(camera2_dm_lens, FLOAT, camera2_lens_dm, radialDistortion, 6);

    //dm, noisereduction
    ASIGN_NEW_MAPPER(camera2_dm_noise, INT32, camera2_noisereduction_dm, mode,     1);
    ASIGN_NEW_MAPPER(camera2_dm_noise, INT8 , camera2_noisereduction_dm, strength, 1);

    //dm, request
    ASIGN_NEW_MAPPER(camera2_dm_request, INT32, camera2_request_dm, frameCount,       1);
    ASIGN_NEW_MAPPER(camera2_dm_request, INT32, camera2_request_dm, id,               1);
    ASIGN_NEW_MAPPER(camera2_dm_request, INT32, camera2_request_dm, metadataMode,     1);
    ASIGN_NEW_MAPPER(camera2_dm_request, INT8,  camera2_request_dm, pipelineDepth,    1);
    ASIGN_NEW_MAPPER(camera2_dm_request, INT32, camera2_request_dm, vendor_frameCount,1);

    //dm, scaler
    ASIGN_NEW_MAPPER(camera2_dm_scaler, INT32, camera2_scaler_dm, cropRegion, 4);

    //dm, sensor
    ASIGN_NEW_MAPPER(camera2_dm_sensor, INT64, camera2_sensor_dm, exposureTime   , 1);
    ASIGN_NEW_MAPPER(camera2_dm_sensor, INT64, camera2_sensor_dm, frameDuration  , 1);
    ASIGN_NEW_MAPPER(camera2_dm_sensor, INT32, camera2_sensor_dm, sensitivity    , 1);
    ASIGN_NEW_MAPPER(camera2_dm_sensor, INT64, camera2_sensor_dm, timeStamp      , 1);
    ASIGN_NEW_MAPPER(camera2_dm_sensor, FLOAT, camera2_sensor_dm, temperature    , 1);
    ASIGN_NEW_MAPPER(camera2_dm_sensor, RATIONAL, camera2_sensor_dm, neutralColorPoint  , 3);
    ASIGN_NEW_MAPPER(camera2_dm_sensor, DOUBLE,camera2_sensor_dm, noiseProfile   , 8);
    ASIGN_NEW_MAPPER(camera2_dm_sensor, FLOAT, camera2_sensor_dm, profileHueSatMap , 24);
    ASIGN_NEW_MAPPER(camera2_dm_sensor, FLOAT, camera2_sensor_dm, profileToneCurve , 64);
    ASIGN_NEW_MAPPER(camera2_dm_sensor, FLOAT, camera2_sensor_dm, greenSplit     , 1);
    ASIGN_NEW_MAPPER(camera2_dm_sensor, INT32, camera2_sensor_dm, testPatternData, 4);
    ASIGN_NEW_MAPPER(camera2_dm_sensor, INT32, camera2_sensor_dm, testPatternMode, 1);
    ASIGN_NEW_MAPPER(camera2_dm_sensor, INT64, camera2_sensor_dm, rollingShutterSkew, 1);

    //dm, shading
    ASIGN_NEW_MAPPER(camera2_dm_shading, INT32, camera2_shading_dm, mode,     1);
    ASIGN_NEW_MAPPER(camera2_dm_shading, INT8 , camera2_shading_dm, strength, 1);

    //dm, stats
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT32, camera2_stats_dm, faceDetectMode    , 1);
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT32, camera2_stats_dm, faceIds           , CAMERA2_MAX_FACES);
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT32, camera2_stats_dm, faceLandmarks     , CAMERA2_MAX_FACES * 6);
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT32, camera2_stats_dm, faceRectangles    , CAMERA2_MAX_FACES * 4);
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT8,  camera2_stats_dm, faceScores        , CAMERA2_MAX_FACES);
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT32, camera2_stats_dm, faceSrcImageSize  , 2);
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT32, camera2_stats_dm, faces             , CAMERA2_MAX_FACES - 2);
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT32, camera2_stats_dm, histogram         , 768);
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT32, camera2_stats_dm, histogramMode     , 1);
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT32, camera2_stats_dm, sharpnessMap      , 12);
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT32, camera2_stats_dm, sharpnessMapMode  , 1);
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT8,  camera2_stats_dm, lensShadingCorrectionMap  , 1);
    ASIGN_NEW_MAPPER(camera2_dm_stats, FLOAT, camera2_stats_dm, lensShadingMap    , 884);
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT32, camera2_stats_dm, sceneFlicker      , 1);
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT32, camera2_stats_dm, hotPixelMapMode   , 1);
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT32, camera2_stats_dm, hotPixelMap       , CAMERA2_MAX_AVAILABLE_MODE * 2);
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT32, camera2_stats_dm, lensShadingMapMode, 1);
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT32, camera2_stats_dm, vendor_lls_tuning_set_index, 1);
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT32, camera2_stats_dm, vendor_lls_brightness_index, 1);
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT32, camera2_stats_dm, vendor_wdrAutoState        , 1);
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT32, camera2_stats_dm, vendor_rgbAvgSamples       , 8);
#ifndef USE_CAMERA_EXYNOS9630_META
#ifndef USE_CAMERA_EXYNOS850_META
#ifndef USE_CAMERA_EXYNOS3830_META
    ASIGN_NEW_MAPPER(camera2_dm_stats, INT32, camera2_stats_dm, vendor_LowLightMode        , 1);
#endif
#endif
#endif

    //dm, tonemap
    ASIGN_NEW_MAPPER(camera2_dm_tonemap, FLOAT, camera2_tonemap_dm, curveBlue , 64);
    ASIGN_NEW_MAPPER(camera2_dm_tonemap, FLOAT, camera2_tonemap_dm, curveGreen, 64);
    ASIGN_NEW_MAPPER(camera2_dm_tonemap, FLOAT, camera2_tonemap_dm, curveRed  , 64);
    ASIGN_NEW_MAPPER(camera2_dm_tonemap, FLOAT, camera2_tonemap_dm, curve     , 1);
    ASIGN_NEW_MAPPER(camera2_dm_tonemap, INT32, camera2_tonemap_dm, mode      , 1);
    ASIGN_NEW_MAPPER(camera2_dm_tonemap, FLOAT, camera2_tonemap_dm, gamma     , 1);
    ASIGN_NEW_MAPPER(camera2_dm_tonemap, INT32, camera2_tonemap_dm, presetCurve, 1);

    //dm, led
    ASIGN_NEW_MAPPER(camera2_dm_led, INT32, camera2_led_dm, transmit, 1);

    //dm, blacklevel
    ASIGN_NEW_MAPPER(camera2_dm_blacklevel, INT32, camera2_blacklevel_dm, lock, 1);

    //dm, sync
    ASIGN_NEW_MAPPER(camera2_dm_sync, INT32, camera2_sync_dm, maxLatency, 1);

    //dm, reprocess
    ASIGN_NEW_MAPPER(camera2_dm_reprocess, FLOAT, camera2_reprocess_dm, effectiveExposureFactor, 1);

    //dm, vendor_entry
    ASIGN_NEW_MAPPER(camera2_dm_vendor_entry, INT32, camera2_entry_dm, lowIndexParam    , 1);
    ASIGN_NEW_MAPPER(camera2_dm_vendor_entry, INT32, camera2_entry_dm, highIndexParam   , 1);

    // uctl
    {
        ASIGN_NEW_MAPPER(camera2_shot_uctl, INT32 , camera2_uctl, uUpdateBitMap  , 1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, INT32 , camera2_uctl, uFrameNumber   , 1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, STRUCT, camera2_uctl, aaUd           , 1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, STRUCT, camera2_uctl, af             , 1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, STRUCT, camera2_uctl, lensUd         , 1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, STRUCT, camera2_uctl, sensorUd       , 1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, STRUCT, camera2_uctl, flashUd        , 1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, STRUCT, camera2_uctl, scalerUd       , 1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, STRUCT, camera2_uctl, isModeUd       , 1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, STRUCT, camera2_uctl, fdUd           , 1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, STRUCT, camera2_uctl, drcUd          , 1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, STRUCT, camera2_uctl, tnrUd          , 1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, STRUCT, camera2_uctl, vtMode         , 1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, FLOAT , camera2_uctl, zoomRatio      , 1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, INT32 , camera2_uctl, flashMode      , 1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, INT32 , camera2_uctl, opMode         , 1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, STRUCT, camera2_uctl, hwlls_mode     , 1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, INT32 , camera2_uctl, statsRoi       , 4);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, INT32 , camera2_uctl, cameraMode     , 1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, INT32 , camera2_uctl, masterCamera   , 1);
#if defined(USE_CAMERA_EXYNOS9630_META) || defined(USE_CAMERA_EXYNOS850_META) || defined(USE_CAMERA_EXYNOS3830_META)
        ASIGN_NEW_MAPPER(camera2_shot_uctl, STRUCT, camera2_uctl, sceneDetectInfoUd,  1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, STRUCT, camera2_uctl, gmvUd      ,       1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, INT32 , camera2_uctl, productColorInfo,  1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, INT8  , camera2_uctl, countryCode,       4);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, INT32 , camera2_uctl, motionState,       1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, INT32 , camera2_uctl, cameraClientIndex, 1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, INT32 , camera2_uctl, remosaicHighResolutionMode, 1);
        ASIGN_NEW_MAPPER(camera2_shot_uctl, INT32 , camera2_uctl, reserved, 6);
#endif

        // uctl, aaUd
        ASIGN_NEW_MAPPER(camera2_uctl_aaUd, STRUCT, camera2_aa_uctl, af_data           , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_aaUd, STRUCT, camera2_aa_uctl, hrmInfo           , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_aaUd, STRUCT, camera2_aa_uctl, illuminationInfo  , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_aaUd, STRUCT, camera2_aa_uctl, gyroInfo          , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_aaUd, STRUCT, camera2_aa_uctl, accInfo           , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_aaUd, STRUCT, camera2_aa_uctl, proximityInfo     , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_aaUd, STRUCT, camera2_aa_uctl, temperatureInfo   , 1);

        // uctl, aaUd, af_data
        ASIGN_NEW_MAPPER(camera2_aa_uctl_af_data, INT32, camera2_obj_af_info, focusState      , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_af_data, INT32, camera2_obj_af_info, focusROILeft    , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_af_data, INT32, camera2_obj_af_info, focusROIRight   , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_af_data, INT32, camera2_obj_af_info, focusROITop     , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_af_data, INT32, camera2_obj_af_info, focusROIBottom  , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_af_data, INT32, camera2_obj_af_info, focusWeight     , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_af_data, INT32, camera2_obj_af_info, w_movement      , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_af_data, INT32, camera2_obj_af_info, h_movement      , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_af_data, INT32, camera2_obj_af_info, w_velocity      , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_af_data, INT32, camera2_obj_af_info, h_velocity      , 1);

        // uctl, aaUd, hrm_sensor_info
        ASIGN_NEW_MAPPER(camera2_aa_uctl_hrmInfo, INT32, camera2_hrm_sensor_info, visible_data , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_hrmInfo, INT32, camera2_hrm_sensor_info, ir_data      , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_hrmInfo, INT32, camera2_hrm_sensor_info, flicker_data , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_hrmInfo, INT32, camera2_hrm_sensor_info, status       , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_hrmInfo, INT32, camera2_hrm_sensor_info, visible_cdata, 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_hrmInfo, INT32, camera2_hrm_sensor_info, visible_rdata, 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_hrmInfo, INT32, camera2_hrm_sensor_info, visible_gdata, 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_hrmInfo, INT32, camera2_hrm_sensor_info, visible_bdata, 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_hrmInfo, INT32, camera2_hrm_sensor_info, ir_gain      , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_hrmInfo, INT32, camera2_hrm_sensor_info, ir_exptime   , 1);

        // uctl, aaUd, illum_sensor_info
        ASIGN_NEW_MAPPER(camera2_aa_uctl_illuminationInfo, INT16, camera2_illuminaion_sensor_info, visible_cdata   , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_illuminationInfo, INT16, camera2_illuminaion_sensor_info, visible_rdata   , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_illuminationInfo, INT16, camera2_illuminaion_sensor_info, visible_gdata   , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_illuminationInfo, INT16, camera2_illuminaion_sensor_info, visible_bdata   , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_illuminationInfo, INT16, camera2_illuminaion_sensor_info, visible_gain    , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_illuminationInfo, INT16, camera2_illuminaion_sensor_info, visible_exptime , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_illuminationInfo, INT16, camera2_illuminaion_sensor_info, ir_north        , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_illuminationInfo, INT16, camera2_illuminaion_sensor_info, ir_south        , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_illuminationInfo, INT16, camera2_illuminaion_sensor_info, ir_east         , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_illuminationInfo, INT16, camera2_illuminaion_sensor_info, ir_west         , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_illuminationInfo, INT16, camera2_illuminaion_sensor_info, ir_gain         , 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_illuminationInfo, INT16, camera2_illuminaion_sensor_info, ir_exptime      , 1);

        // uctl, aaUd, gyro_sensor_info
        ASIGN_NEW_MAPPER(camera2_aa_uctl_gyroInfo, FLOAT, camera2_gyro_sensor_info, x, 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_gyroInfo, FLOAT, camera2_gyro_sensor_info, y, 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_gyroInfo, FLOAT, camera2_gyro_sensor_info, z, 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_gyroInfo, INT32, camera2_gyro_sensor_info, state, 1);

        // uctl, aaUd, accelerometer_sensor_info
        ASIGN_NEW_MAPPER(camera2_aa_uctl_accInfo, FLOAT, camera2_accelerometer_sensor_info, x, 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_accInfo, FLOAT, camera2_accelerometer_sensor_info, y, 1);
        ASIGN_NEW_MAPPER(camera2_aa_uctl_accInfo, FLOAT, camera2_accelerometer_sensor_info, z, 1);

        // uctl, aaUd, proximity_sensor_info
        ASIGN_NEW_MAPPER(camera2_aa_uctl_proximityInfo, INT32, camera2_proximity_sensor_info, flicker_data, 1);

        // uctl, aaUd, temperature_info
        ASIGN_NEW_MAPPER(camera2_aa_uctl_temperatureInfo, INT32, camera2_temperature_info, usb_thermal, 1);

        // uctl, af
        ASIGN_NEW_MAPPER(camera2_uctl_af, INT32, camera2_af_uctl, vs2Length       , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_af, INT32, camera2_af_uctl, vendorSpecific2       , CAMERA2_MAX_UCTL_VENDOR2_LENGTH);

        // uctl, lensUd
        ASIGN_NEW_MAPPER(camera2_uctl_lensUd, INT32, camera2_lens_uctl, pos       , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_lensUd, INT32, camera2_lens_uctl, posSize   , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_lensUd, INT32, camera2_lens_uctl, direction , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_lensUd, INT32, camera2_lens_uctl, slewRate  , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_lensUd, INT32, camera2_lens_uctl, oisCoefVal, 1);

        // uctl, sensorUd
        ASIGN_NEW_MAPPER(camera2_uctl_sensorUd, INT64, camera2_sensor_uctl, dynamicFrameDuration , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_sensorUd, INT32, camera2_sensor_uctl, analogGain           , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_sensorUd, INT32, camera2_sensor_uctl, digitalGain          , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_sensorUd, INT64, camera2_sensor_uctl, longExposureTime     , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_sensorUd, INT64, camera2_sensor_uctl, shortExposureTime    , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_sensorUd, INT32, camera2_sensor_uctl, longAnalogGain       , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_sensorUd, INT32, camera2_sensor_uctl, shortAnalogGain      , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_sensorUd, INT32, camera2_sensor_uctl, longDigitalGain      , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_sensorUd, INT32, camera2_sensor_uctl, shortDigitalGain     , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_sensorUd, INT64, camera2_sensor_uctl, exposureTime         , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_sensorUd, INT32, camera2_sensor_uctl, frameDuration        , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_sensorUd, INT32, camera2_sensor_uctl, sensitivity          , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_sensorUd, INT32, camera2_sensor_uctl, CtrlFrameDuration    , 1);

        // uctl, flashUd
        ASIGN_NEW_MAPPER(camera2_uctl_flashUd, INT32, camera2_flash_uctl, firingPower , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_flashUd, INT64, camera2_flash_uctl, firingTime  , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_flashUd, INT32, camera2_flash_uctl, flashMode   , 1);

        // uctl, scaler
        ASIGN_NEW_MAPPER(camera2_uctl_flashUd, INT32, camera2_scaler_uctl, orientation   , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_flashUd, INT32, camera2_scaler_uctl, mcsc_sub_blk_port   , INTERFACE_TYPE_MAX);

        // uctl, isModeUd
        ASIGN_NEW_MAPPER(camera2_uctl_isModeUd, INT32, camera2_is_mode_uctl, wdr_mode       , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_isModeUd, INT32, camera2_is_mode_uctl, paf_mode       , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_isModeUd, INT32, camera2_is_mode_uctl, disparity_mode , 1);

        // uctl, fdUd
        ASIGN_NEW_MAPPER(camera2_uctl_fdUd, INT32, camera2_fd_uctl, faceDetectMode , 1);
        ASIGN_NEW_MAPPER(camera2_uctl_fdUd, INT32, camera2_fd_uctl, faceIds        , CAMERA2_MAX_FACES);
        ASIGN_NEW_MAPPER(camera2_uctl_fdUd, INT32, camera2_fd_uctl, faceLandmarks  , CAMERA2_MAX_FACES * 6);
        ASIGN_NEW_MAPPER(camera2_uctl_fdUd, INT32, camera2_fd_uctl, faceRectangles , CAMERA2_MAX_FACES * 4);
        ASIGN_NEW_MAPPER(camera2_uctl_fdUd, INT8 , camera2_fd_uctl, faceScores     , CAMERA2_MAX_FACES);
        ASIGN_NEW_MAPPER(camera2_uctl_fdUd, INT32, camera2_fd_uctl, faces          , CAMERA2_MAX_FACES);
        ASIGN_NEW_MAPPER(camera2_uctl_fdUd, INT32, camera2_fd_uctl, vendorSpecific , CAMERA2_MAX_UCTL_VENDER_LENGTH);

        // uctl, drcUd
        ASIGN_NEW_MAPPER(camera2_uctl_drcUd, INT32, camera2_drc_uctl, uDrcEn, 1);

        // uclt, dcpUd
        // ASIGN_NEW_MAPPER(camera2_uctl_dcpUd, STRUCT, camera2_dcp_uctl, wide_gdc_grid_value, 1);
        // ASIGN_NEW_MAPPER(camera2_uctl_dcpUd, STRUCT, camera2_dcp_uctl, tele_gdc_grid_value, 1);
        // ASIGN_NEW_MAPPER(camera2_uctl_dcpUd, STRUCT, camera2_dcp_uctl, wide_gdc_upscale_size, 1);
        // ASIGN_NEW_MAPPER(camera2_uctl_dcpUd, STRUCT, camera2_dcp_uctl, tele_gdc_upscale_size, 1);
        // ASIGN_NEW_MAPPER(camera2_uctl_dcpUd, STRUCT, camera2_dcp_uctl, wide_gamma_LUT, 1);
        // ASIGN_NEW_MAPPER(camera2_uctl_dcpUd, STRUCT, camera2_dcp_uctl, tele_gamma_LUT, 1);

        // uctl, dcpUd, wide_gdc_grid_value
        // ASIGN_NEW_MAPPER(camera2_dcp_uctl_wide_gdc_grid_value, INT32, camera2_dcp_homo_matrix, dx, 63);
        // ASIGN_NEW_MAPPER(camera2_dcp_uctl_wide_gdc_grid_value, INT32, camera2_dcp_homo_matrix, dy, 63);

        // uctl, dcpUd, tele_gdc_grid_value
        // camera2_dcp_homo_matrix_dx = new MetaMapper(MAPPER_PARAM(INT32,camera2_dcp_homo_matrix,dx,63));
        // camera2_dcp_uctl_tele_gdc_grid_value->child[MAKE_STRING(INT32)] = camera2_dcp_homo_matrix_dx;
        // camera2_dcp_homo_matrix_dy = new MetaMapper(MAPPER_PARAM(INT32,camera2_dcp_homo_matrix,dy,63));
        // camera2_dcp_uctl_tele_gdc_grid_value->child[MAKE_STRING(INT32)] = camera2_dcp_homo_matrix_dy;

        // uctl, dcpUd, wide_gdc_upscale_size
        // ASIGN_NEW_MAPPER(camera2_dcp_uctl_wide_gdc_upscale_size, INT32, camera2_area,x, 1);
        // ASIGN_NEW_MAPPER(camera2_dcp_uctl_wide_gdc_upscale_size, INT32, camera2_area,y, 1);
        // ASIGN_NEW_MAPPER(camera2_dcp_uctl_wide_gdc_upscale_size, INT32, camera2_area,w, 1);
        // ASIGN_NEW_MAPPER(camera2_dcp_uctl_wide_gdc_upscale_size, INT32, camera2_area,h, 1);

       // uctl, dcpUd, tele_gdc_upscale_size
        // camera2_area_x = new MetaMapper(MAPPER_PARAM(INT32,camera2_area,x,1));
        // camera2_dcp_uctl_tele_gdc_upscale_size->child[MAKE_STRING(INT32)] = camera2_area_x;
        // camera2_area_y = new MetaMapper(MAPPER_PARAM(INT32,camera2_area,x,1));
        // camera2_dcp_uctl_tele_gdc_upscale_size->child[MAKE_STRING(INT32)] = camera2_area_y;
        // camera2_area_w = new MetaMapper(MAPPER_PARAM(INT32,camera2_area,w,1));
        // camera2_dcp_uctl_tele_gdc_upscale_size->child[MAKE_STRING(INT32)] = camera2_area_w;
        // camera2_area_h = new MetaMapper(MAPPER_PARAM(INT32,camera2_area,h,1));
        // camera2_dcp_uctl_tele_gdc_upscale_size->child[MAKE_STRING(INT32)] = camera2_area_h;

        // uctl, dcpUd, wide_gamma_LUT
        // ASIGN_NEW_MAPPER(camera2_dcp_uctl_wide_gamma_LUT, INT32, camera2_dcp_rgb_gamma_lut,x_LUT, 32);
        // ASIGN_NEW_MAPPER(camera2_dcp_uctl_wide_gamma_LUT, INT32, camera2_dcp_rgb_gamma_lut,r_LUT, 32);
        // ASIGN_NEW_MAPPER(camera2_dcp_uctl_wide_gamma_LUT, INT32, camera2_dcp_rgb_gamma_lut,g_LUT, 32);
        // ASIGN_NEW_MAPPER(camera2_dcp_uctl_wide_gamma_LUT, INT32, camera2_dcp_rgb_gamma_lut,b_LUT, 32);

        // uctl, dcpUd, tele_gamma_LUT
        // ASIGN_NEW_MAPPER(camera2_dcp_uctl_wide_gamma_LUT, INT32, camera2_dcp_rgb_gamma_lut,  mode, 1);
        // camera2_dcp_rgb_gamma_lut_x_LUT = new MetaMapper(MAPPER_PARAM(INT32,camera2_dcp_rgb_gamma_lut,x_LUT,32));
        // camera2_dcp_uctl_tele_gamma_LUT->child[MAKE_STRING(INT32)] = camera2_dcp_rgb_gamma_lut_x_LUT;
        // camera2_dcp_rgb_gamma_lut_r_LUT = new MetaMapper(MAPPER_PARAM(INT32,camera2_dcp_rgb_gamma_lut,r_LUT,32));
        // camera2_dcp_uctl_tele_gamma_LUT->child[MAKE_STRING(INT32)] = camera2_dcp_rgb_gamma_lut_r_LUT;
        // camera2_dcp_rgb_gamma_lut_g_LUT = new MetaMapper(MAPPER_PARAM(INT32,camera2_dcp_rgb_gamma_lut,g_LUT,32));
        // camera2_dcp_uctl_tele_gamma_LUT->child[MAKE_STRING(INT32)] = camera2_dcp_rgb_gamma_lut_g_LUT;
        // camera2_dcp_rgb_gamma_lut_b_LUT = new MetaMapper(MAPPER_PARAM(INT32,camera2_dcp_rgb_gamma_lut,b_LUT,32));
        // camera2_dcp_uctl_tele_gamma_LUT->child[MAKE_STRING(INT32)] = camera2_dcp_rgb_gamma_lut_b_LUT;

        // uctl, sceneDetectInfoUd
        ASIGN_NEW_MAPPER(camera2_uctl_sceneDetectInfoUd, INT64, camera2_scene_detect_uctl,timeStamp, 1);
        ASIGN_NEW_MAPPER(camera2_uctl_sceneDetectInfoUd, INT32, camera2_scene_detect_uctl,scene_index,1);
        ASIGN_NEW_MAPPER(camera2_uctl_sceneDetectInfoUd, INT32, camera2_scene_detect_uctl,confidence_score, 1);
        ASIGN_NEW_MAPPER(camera2_uctl_sceneDetectInfoUd, INT32, camera2_scene_detect_uctl,object_roi, 4);

        // uctl, hwlls_mode
        ASIGN_NEW_MAPPER(camera2_uctl_hwlls_mode, INT32, camera2_is_hw_lls_uctl, hwLlsMode,     1);
        ASIGN_NEW_MAPPER(camera2_uctl_hwlls_mode, INT32, camera2_is_hw_lls_uctl, hwLlsProgress, 1);
    }

    // udm
    {
        ASIGN_NEW_MAPPER(camera2_shot_udm, STRUCT, camera2_udm, aa           , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, STRUCT, camera2_udm, lens         , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, STRUCT, camera2_udm, sensor       , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, STRUCT, camera2_udm, flash        , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, STRUCT, camera2_udm, ae           , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, STRUCT, camera2_udm, awb          , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, STRUCT, camera2_udm, af           , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, STRUCT, camera2_udm, as           , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, STRUCT, camera2_udm, ipc          , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, STRUCT, camera2_udm, rta          , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, STRUCT, camera2_udm, internal     , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, STRUCT, camera2_udm, scaler       , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, STRUCT, camera2_udm, isMode       , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, STRUCT, camera2_udm, pdaf         , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, STRUCT, camera2_udm, fd           , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, STRUCT, camera2_udm, me           , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, INT32 , camera2_udm, vtMode       , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, FLOAT , camera2_udm, zoomRatio    , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, INT32 , camera2_udm, flashMode    , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, INT32 , camera2_udm, opMode       , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, STRUCT, camera2_udm, ni           , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, STRUCT, camera2_udm, drc          , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, STRUCT, camera2_udm, rgbGamma     , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, STRUCT, camera2_udm, ccm          , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, INT32 , camera2_udm, cameraMode   , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, INT32 , camera2_udm, masterCamera , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, INT32 , camera2_udm, fallback     , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, INT32 , camera2_udm, frame_id     , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, INT32 , camera2_udm, scene_index  , 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, INT32 , camera2_udm, flicker_detect, 1);
        ASIGN_NEW_MAPPER(camera2_shot_udm, INT32 , camera2_udm, reserved     , 7);

        // udm, aa
        ASIGN_NEW_MAPPER(camera2_udm_aa, STRUCT, camera2_aa_udm, af_data           , 1);
        ASIGN_NEW_MAPPER(camera2_udm_aa, STRUCT, camera2_aa_udm, hrmInfo           , 1);
        ASIGN_NEW_MAPPER(camera2_udm_aa, STRUCT, camera2_aa_udm, illuminationInfo  , 1);
        ASIGN_NEW_MAPPER(camera2_udm_aa, STRUCT, camera2_aa_udm, gyroInfo          , 1);
        ASIGN_NEW_MAPPER(camera2_udm_aa, STRUCT, camera2_aa_udm, accInfo           , 1);
        ASIGN_NEW_MAPPER(camera2_udm_aa, STRUCT, camera2_aa_udm, proximityInfo     , 1);
        ASIGN_NEW_MAPPER(camera2_udm_aa, STRUCT, camera2_aa_udm, temperatureInfo   , 1);

        // udm, aa, af_data
        ASIGN_NEW_MAPPER(camera2_aa_udm_af_data, INT32, camera2_obj_af_info, focusState      , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_af_data, INT32, camera2_obj_af_info, focusROILeft    , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_af_data, INT32, camera2_obj_af_info, focusROIRight   , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_af_data, INT32, camera2_obj_af_info, focusROITop     , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_af_data, INT32, camera2_obj_af_info, focusROIBottom  , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_af_data, INT32, camera2_obj_af_info, focusWeight     , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_af_data, INT32, camera2_obj_af_info, w_movement      , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_af_data, INT32, camera2_obj_af_info, h_movement      , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_af_data, INT32, camera2_obj_af_info, w_velocity      , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_af_data, INT32, camera2_obj_af_info, h_velocity      , 1);

        // udm, aa, hrm_sensor_info
        ASIGN_NEW_MAPPER(camera2_aa_udm_hrmInfo, INT32, camera2_hrm_sensor_info, visible_data  , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_hrmInfo, INT32, camera2_hrm_sensor_info, ir_data       , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_hrmInfo, INT32, camera2_hrm_sensor_info, flicker_data  , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_hrmInfo, INT32, camera2_hrm_sensor_info, status        , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_hrmInfo, INT32, camera2_hrm_sensor_info, visible_cdata , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_hrmInfo, INT32, camera2_hrm_sensor_info, visible_rdata , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_hrmInfo, INT32, camera2_hrm_sensor_info, visible_gdata , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_hrmInfo, INT32, camera2_hrm_sensor_info, visible_bdata , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_hrmInfo, INT32, camera2_hrm_sensor_info, ir_gain , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_hrmInfo, INT32, camera2_hrm_sensor_info, ir_exptime      , 1);

        // udm, aa, illum_sensor_info
        ASIGN_NEW_MAPPER(camera2_aa_udm_illuminationInfo, INT16, camera2_illuminaion_sensor_info, visible_cdata   , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_illuminationInfo, INT16, camera2_illuminaion_sensor_info, visible_rdata   , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_illuminationInfo, INT16, camera2_illuminaion_sensor_info, visible_gdata   , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_illuminationInfo, INT16, camera2_illuminaion_sensor_info, visible_bdata   , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_illuminationInfo, INT16, camera2_illuminaion_sensor_info, visible_gain    , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_illuminationInfo, INT16, camera2_illuminaion_sensor_info, visible_exptime , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_illuminationInfo, INT16, camera2_illuminaion_sensor_info, ir_north        , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_illuminationInfo, INT16, camera2_illuminaion_sensor_info, ir_south        , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_illuminationInfo, INT16, camera2_illuminaion_sensor_info, ir_east         , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_illuminationInfo, INT16, camera2_illuminaion_sensor_info, ir_west         , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_illuminationInfo, INT16, camera2_illuminaion_sensor_info, ir_gain         , 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_illuminationInfo, INT16, camera2_illuminaion_sensor_info, ir_exptime      , 1);

        // udm, aa, gyro_sensor_info
        ASIGN_NEW_MAPPER(camera2_aa_udm_gyroInfo, FLOAT, camera2_gyro_sensor_info, x, 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_gyroInfo, FLOAT, camera2_gyro_sensor_info, y, 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_gyroInfo, FLOAT, camera2_gyro_sensor_info, z, 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_gyroInfo, INT32, camera2_gyro_sensor_info,state, 1);

        // udm, aa, accelerometer_sensor_info
        ASIGN_NEW_MAPPER(camera2_aa_udm_accInfo, FLOAT, camera2_accelerometer_sensor_info,x, 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_accInfo, FLOAT, camera2_accelerometer_sensor_info,y, 1);
        ASIGN_NEW_MAPPER(camera2_aa_udm_accInfo, FLOAT, camera2_accelerometer_sensor_info,z, 1);

        // udm, aa, proximity_sensor_info
        ASIGN_NEW_MAPPER(camera2_aa_udm_proximityInfo, INT32, camera2_proximity_sensor_info, flicker_data, 1);

        // udm, aa, temperature_info
        ASIGN_NEW_MAPPER(camera2_aa_udm_temperatureInfo, INT32, camera2_temperature_info, usb_thermal, 1);

        // udm, lens
        ASIGN_NEW_MAPPER(camera2_udm_lens, INT32, camera2_lens_udm, pos       , 1);
        ASIGN_NEW_MAPPER(camera2_udm_lens, INT32, camera2_lens_udm, posSize   , 1);
        ASIGN_NEW_MAPPER(camera2_udm_lens, INT32, camera2_lens_udm, direction , 1);
        ASIGN_NEW_MAPPER(camera2_udm_lens, INT32, camera2_lens_udm, slewRate  , 1);
        ASIGN_NEW_MAPPER(camera2_udm_lens, INT32, camera2_lens_udm, oisCoefVal, 1);

        // udm, sensor
        ASIGN_NEW_MAPPER(camera2_udm_sensor, INT64, camera2_sensor_udm, dynamicFrameDuration , 1);
        ASIGN_NEW_MAPPER(camera2_udm_sensor, INT32, camera2_sensor_udm, analogGain           , 1);
        ASIGN_NEW_MAPPER(camera2_udm_sensor, INT32, camera2_sensor_udm, digitalGain          , 1);
        ASIGN_NEW_MAPPER(camera2_udm_sensor, INT64, camera2_sensor_udm, longExposureTime     , 1);
        ASIGN_NEW_MAPPER(camera2_udm_sensor, INT64, camera2_sensor_udm, midExposureTime     , 1);
        ASIGN_NEW_MAPPER(camera2_udm_sensor, INT64, camera2_sensor_udm, shortExposureTime    , 1);
        ASIGN_NEW_MAPPER(camera2_udm_sensor, INT32, camera2_sensor_udm, longAnalogGain       , 1);
        ASIGN_NEW_MAPPER(camera2_udm_sensor, INT32, camera2_sensor_udm, midAnalogGain       , 1);
        ASIGN_NEW_MAPPER(camera2_udm_sensor, INT32, camera2_sensor_udm, shortAnalogGain      , 1);
        ASIGN_NEW_MAPPER(camera2_udm_sensor, INT32, camera2_sensor_udm, longDigitalGain      , 1);
        ASIGN_NEW_MAPPER(camera2_udm_sensor, INT32, camera2_sensor_udm, midDigitalGain      , 1);
        ASIGN_NEW_MAPPER(camera2_udm_sensor, INT32, camera2_sensor_udm, shortDigitalGain     , 1);
        ASIGN_NEW_MAPPER(camera2_udm_sensor, INT32, camera2_sensor_udm, shortWdrExposureTime , 1);
        ASIGN_NEW_MAPPER(camera2_udm_sensor, INT32, camera2_sensor_udm, longWdrExposureTime  , 1);
        ASIGN_NEW_MAPPER(camera2_udm_sensor, INT32, camera2_sensor_udm, shortWdrSensitivity  , 1);
        ASIGN_NEW_MAPPER(camera2_udm_sensor, INT32, camera2_sensor_udm, longWdrSensitivity   , 1);
        ASIGN_NEW_MAPPER(camera2_udm_sensor, INT64, camera2_sensor_udm, timeStampBoot        , 1);
        ASIGN_NEW_MAPPER(camera2_udm_sensor, INT32, camera2_sensor_udm, multiLuminances      , 9);

        // udm, flash
        ASIGN_NEW_MAPPER(camera2_udm_flash, INT32, camera2_flash_udm, firingPower , 1);
        ASIGN_NEW_MAPPER(camera2_udm_flash, INT64, camera2_flash_udm, firingTime  , 1);
        ASIGN_NEW_MAPPER(camera2_udm_flash, INT32, camera2_flash_udm, flashMode   , 1);

        // udm, ae
        ASIGN_NEW_MAPPER(camera2_udm_ae, INT32, camera2_ae_udm, vsLength   , 1);
        ASIGN_NEW_MAPPER(camera2_udm_ae, INT32, camera2_ae_udm, vendorSpecific   , CAMERA2_MAX_VENDER_LENGTH);
        ASIGN_NEW_MAPPER(camera2_udm_ae, INT32, camera2_ae_udm, vs2Length   , 1);
        ASIGN_NEW_MAPPER(camera2_udm_ae, INT32, camera2_ae_udm, vendorSpecific2  , CAMERA2_MAX_UDM_VENDOR2_LENGTH);

        // udm, awb
        ASIGN_NEW_MAPPER(camera2_udm_awb, INT32, camera2_awb_udm, vsLength   , 1);
        ASIGN_NEW_MAPPER(camera2_udm_awb, INT32, camera2_awb_udm, vendorSpecific   , CAMERA2_MAX_VENDER_LENGTH);
        ASIGN_NEW_MAPPER(camera2_udm_awb, INT32, camera2_awb_udm, vs2Length   , 1);
        ASIGN_NEW_MAPPER(camera2_udm_awb, INT32, camera2_awb_udm, vendorSpecific2  , CAMERA2_MAX_UDM_VENDOR2_LENGTH);


        // udm, af
        ASIGN_NEW_MAPPER(camera2_udm_af, INT32, camera2_af_udm, vsLength   , 1);
        ASIGN_NEW_MAPPER(camera2_udm_af, INT32, camera2_af_udm, vendorSpecific   , CAMERA2_MAX_VENDER_LENGTH);
        ASIGN_NEW_MAPPER(camera2_udm_af, INT32, camera2_af_udm, lensPositionInfinity , 1);
        ASIGN_NEW_MAPPER(camera2_udm_af, INT32, camera2_af_udm, lensPositionMacro   , 1);
        ASIGN_NEW_MAPPER(camera2_udm_af, INT32, camera2_af_udm, lensPositionCurrent , 1);
        ASIGN_NEW_MAPPER(camera2_udm_af, INT32, camera2_af_udm, vs2Length   , 1);
        ASIGN_NEW_MAPPER(camera2_udm_af, INT32, camera2_af_udm, vendorSpecific2  , CAMERA2_MAX_UDM_VENDOR2_LENGTH-1);

        // udm, as
        ASIGN_NEW_MAPPER(camera2_udm_as, INT32, camera2_as_udm, vsLength   , 1);
        ASIGN_NEW_MAPPER(camera2_udm_as, INT32, camera2_as_udm, vendorSpecific   , CAMERA2_MAX_VENDER_LENGTH);


        // udm, ipc
        ASIGN_NEW_MAPPER(camera2_udm_ipc,INT32, camera2_ipc_udm, vsLength   , 1);
        ASIGN_NEW_MAPPER(camera2_udm_ipc,INT32, camera2_ipc_udm, vendorSpecific   , CAMERA2_MAX_IPC_VENDER_LENGTH);

        // udm, rta
        ASIGN_NEW_MAPPER(camera2_udm_rta, INT32, camera2_rta_udm, vsLength   , 1);
        ASIGN_NEW_MAPPER(camera2_udm_rta, INT32, camera2_rta_udm, vendorSpecific , 90);
        ASIGN_NEW_MAPPER(camera2_udm_rta, INT32, camera2_rta_udm, vs2Length   , 1);
        ASIGN_NEW_MAPPER(camera2_udm_rta, INT32, camera2_rta_udm, vendorSpecific2  , 90);
        // udm, internal

        ASIGN_NEW_MAPPER(camera2_udm_internal, INT32, camera2_internal_udm, ProcessedFrameInfo   , 1);
        ASIGN_NEW_MAPPER(camera2_udm_internal, INT32, camera2_internal_udm, vendorSpecific  , 4);

        // udm, scaler, ysum
        ASIGN_NEW_MAPPER(camera2_udm_scaler, STRUCT, camera2_scaler_udm, ysumdata , 1);
        ASIGN_NEW_MAPPER(camera2_scaler_udm_ysumdata, INT32, ysum_data, lower_ysum_value, 1);
        ASIGN_NEW_MAPPER(camera2_scaler_udm_ysumdata, INT32, ysum_data, higher_ysum_value, 1);

        // udm, isMode
        ASIGN_NEW_MAPPER(camera2_udm_isMode, INT32, camera2_is_mode_udm, wdr_mode       , 1);
        ASIGN_NEW_MAPPER(camera2_udm_isMode, INT32, camera2_is_mode_udm, paf_mode       , 1);
        ASIGN_NEW_MAPPER(camera2_udm_isMode, INT32, camera2_is_mode_udm, disparity_mode , 1);

        // udm, pdaf
        ASIGN_NEW_MAPPER(camera2_udm_pdaf, INT16, camera2_pdaf_udm, numCol           , 1);
        ASIGN_NEW_MAPPER(camera2_udm_pdaf, INT16, camera2_pdaf_udm, numRow           , 1);
        //Can't support(too large..)
        //ASIGN_NEW_MAPPER(camera2_udm_pdaf, STRUCT, camera2_pdaf_udm, multiResult       , 1);
        //ASIGN_NEW_MAPPER(camera2_udm_pdaf, STRUCT, camera2_pdaf_udm, singleResult      , 1);
        ASIGN_NEW_MAPPER(camera2_udm_pdaf, INT16, camera2_pdaf_udm, lensPosResolution , 1);

        // udm, fd
        ASIGN_NEW_MAPPER(camera2_udm_fd, INT32, camera2_fd_udm, faceCount , 1);
        ASIGN_NEW_MAPPER(camera2_udm_fd, INT32, camera2_fd_udm, vendorSpecific , CAMERA2_MAX_UCTL_VENDER_LENGTH);

        // udm, me
        ASIGN_NEW_MAPPER(camera2_udm_me, INT32, camera2_me_udm, motion_vector , CAMERA2_MAX_ME_MV);
        ASIGN_NEW_MAPPER(camera2_udm_me, INT32, camera2_me_udm, current_patch, CAMERA2_MAX_ME_MV);

        // udm, ni
        ASIGN_NEW_MAPPER(camera2_udm_ni, INT32, camera2_ni_udm, currentFrameNoiseIndex  , 1);
        ASIGN_NEW_MAPPER(camera2_udm_ni, INT32, camera2_ni_udm, nextFrameNoiseIndex     , 1);
        ASIGN_NEW_MAPPER(camera2_udm_ni, INT32, camera2_ni_udm, nextNextFrameNoiseIndex , 1);

        // udm, drc
        ASIGN_NEW_MAPPER(camera2_udm_drc, INT16, camera2_drc_udm, globalToneMap, 32);

        // udm, rgbGamma
        ASIGN_NEW_MAPPER(camera2_udm_rgbGamma, INT16, camera2_rgbGamma_udm, xTable, 32);
        ASIGN_NEW_MAPPER(camera2_udm_rgbGamma, INT16, camera2_rgbGamma_udm, yTable, 3 * 32);

        // udm, ccm
        ASIGN_NEW_MAPPER(camera2_udm_ccm, INT16, camera2_ccm_udm, ccmVectors9, 27);
    }
}

void ExynosCameraLogManager::m_panic(void)
{
    ALOGE("panic..");
}

void ExynosCameraLogManager::m_printLog(android_LogPriority prio, const char *tag, String8& logStr)
{
    switch(prio) {
    case ANDROID_LOG_FATAL:
        android_printAssert(NULL, tag, "%s", logStr.c_str());
        break;
    default:
        LOG_PRI(prio, tag, "%s", logStr.c_str());
        break;
    }
}

void ExynosCameraLogManager::m_logDebug(android_LogPriority &prio, const char *tag, String8& logStr)
{
    status_t ret;
    std::string propValue;
    bool enable;

    /* 1. assert check */
    if (prio == ANDROID_LOG_FATAL) {
        ret = m_property.get(ExynosCameraProperty::DEBUG_ASSERT, tag, propValue);
        if (ret != NO_ERROR) return;

        if (propValue.compare("panic") == 0) {
            // after print this log, do panic
            m_printLog(prio, tag, logStr);
            m_panic();
        } else if (propValue.compare("errlog") == 0) {
            // change log level
            prio = ANDROID_LOG_ERROR;
        //TBD : } else if (propValue.compare("file") == 0) {
        }
    }

    /* 2. trap check */
    int trapCount;

    ret = m_property.get(ExynosCameraProperty::DEBUG_TRAP_ENABLE, tag, enable);
    if (ret != NO_ERROR) return;
    if (!enable) return;

    ret = m_property.get(ExynosCameraProperty::DEBUG_TRAP_COUNT, tag, trapCount);
    if (ret != NO_ERROR) return;

    std::vector<std::string> sV;
    ret = m_property.getVector(ExynosCameraProperty::DEBUG_TRAP_WORDS, tag, sV);
    if (ret != NO_ERROR) return;

    if (sV.size() > 0) {
        bool find = false;
        for (std::string each : sV) {
            if (logStr.find(each.c_str()) >= 0) {
                find = true;
                break;
            }
        }

        if (find) {
            LOG_PRI(ANDROID_LOG_ERROR, tag, "TRAP(%d)!!: %s", trapCount, logStr.c_str());

            if ((--trapCount) <= 0) {
                trapCount = 0;

                // trap!!
                // get the trap event
                ret = m_property.get(ExynosCameraProperty::DEBUG_TRAP_EVENT, tag, propValue);
                if (ret != NO_ERROR) return;

                if (propValue.compare("panic") == 0) {
                    // after print this log, do panic
                    m_panic();
                } else if (propValue.compare("assert") == 0) {
                    // change log level
                    prio = ANDROID_LOG_FATAL;
                    //TBD : } else if (propValue.compare("file") == 0) {
                }
            }

            // update trap count
            ret = m_property.set(ExynosCameraProperty::DEBUG_TRAP_COUNT, tag,
                    std::to_string(trapCount).c_str());
            if (ret != NO_ERROR) return;
        }
    }
}

void ExynosCameraLogManager::m_logPerformanceFps(android_LogPriority prio,
            const char* tag,
            const char* prefix,
            int cameraId,
            int factoryType,
            PerformanceType pType,
            TimeCalcType tType,
            int pos,
            int requestKey,
            void *frame)
{
    status_t ret;
    std::string propValue;

    int32_t numFrame;
    int64_t prevKey = 0;
    PerformancePos prevPos = PERF_FPS_SERVICE_REQUEST;
    int64_t myKey = m_makeTimestampKey(cameraId, factoryType, requestKey);
    int64_t summaryKey = m_makeSummaryKey(cameraId, factoryType, tType, pos);
    nsecs_t lastTimestamp = 0LL;
    nsecs_t curTimestamp = systemTime(SYSTEM_TIME_BOOTTIME);
    TimeInfo summary = m_perfSummary[pType][summaryKey];

    Mutex::Autolock l(m_perfLock[pType]);

    switch (pos) {
    case PERF_FPS_SERVICE_REQUEST:
        break;
    case PERF_FPS_FRAME_CREATE:
    case PERF_FPS_PATIAL_RESULT:
    case PERF_FPS_ALL_RESULT:
        prevPos = PERF_FPS_SERVICE_REQUEST;
        prevKey = m_makeTimestampKey(cameraId, 0, requestKey);
        break;
    case PERF_FPS_FRAME_COMPLETE:
        prevPos = PERF_FPS_FRAME_CREATE;
        prevKey = m_makeTimestampKey(cameraId, factoryType, requestKey);
        {
            ExynosCameraFrame *framePtr = static_cast<ExynosCameraFrame *>(frame);
            if (framePtr == nullptr) return;

            // error counting
            switch (framePtr->getPreviousFrameState()) {
            case FRAME_STATE_SKIPPED:
            case FRAME_STATE_INVALID:
                summary.errCnt++;
                // exit
                goto p_exit;
                break;
            default:
                break;
            }

            // traverse all sibling entry (only to get durations of pair of pipes)
            ExynosCameraFrameEntity *entity = framePtr->getFirstEntity();

            while (entity != nullptr) {
                TimeSubInfo_sp_t sub = summary.sub;

                if (entity->getEntityState() == ENTITY_STATE_FRAME_DONE ||
                    entity->getEntityState() == ENTITY_STATE_COMPLETE) {
                    int32_t pipeId = entity->getPipeId();
                    nsecs_t processTimestamp = entity->getProcessTimestamp();
                    nsecs_t doneTimestamp = entity->getDoneTimestamp();
                    nsecs_t diff = (doneTimestamp - processTimestamp) / 1000LL; // converting usec

                    // duration of mine, min, max, tot
                    TimeUnitInfo duration = sub->duration[pipeId];
                    duration.count++;
                    if (duration.min == 0) duration.min = 0xFFFFFFFF;
                    duration.min = MIN(diff, duration.min);
                    duration.max = MAX(diff, duration.max);
                    duration.sum += diff;
                    sub->duration[pipeId] = duration;

                    // Internal min, max, tot
                    TimeUnitInfo interval = sub->interval[pipeId];
                    if (interval.lastTimestamp != 0LL) {
                        nsecs_t prevDiff = (doneTimestamp - interval.lastTimestamp) / 1000LL;
                        interval.count++;
                        if (interval.min == 0) interval.min = 0xFFFFFFFF;
                        interval.min = MIN(prevDiff, interval.min);
                        interval.max = MAX(prevDiff, interval.max);
                        interval.sum += prevDiff;
                    }
                    interval.lastTimestamp = doneTimestamp;
                    sub->interval[pipeId] = interval;

                }
                entity = framePtr->getNextEntity();
            }
        }
        break;
    default:
        return;
    }

    switch (tType) {
    case TM_DURATION:
        lastTimestamp = m_perfTimestamp[pType][prevPos][prevKey];
        break;
    case TM_INTERVAL:
        lastTimestamp = m_perfTimestamp[pType][pos][tType];
        break;
    default:
        break;
    }
    {
        // calulate interval or duration time
        nsecs_t diff = (curTimestamp - lastTimestamp) / 1000LL;
        summary.time.count++;
        if (summary.time.min == 0) summary.time.min = 0xFFFFFFFF;
        summary.time.min = MIN(diff, summary.time.min);
        summary.time.max = MAX(diff, summary.time.max);
        summary.time.sum += diff;
    }

    // update lastTimestamp
    m_perfTimestamp[pType][pos][tType] = curTimestamp;
    m_perfTimestamp[pType][pos][myKey] = curTimestamp;

p_exit:
    ret = m_property.get(ExynosCameraProperty::LOG_PERFORMANCE_FPS_FRAMES, tag, numFrame);
    if (ret != NO_ERROR) return;

    summary.count++;
    m_perfSummary[pType][summaryKey] = summary;

    // print summary
    if (summary.count % numFrame == 0) {
        summary.index++;

        String8 prefixLogStr;
        String8 mainLogStr;

        ret = prefixLogStr.appendFormat(prefix, cameraId, "");

        switch (tType) {
        case TM_DURATION:
            ret = prefixLogStr.appendFormat("[FACTORY(%c)/%d.%s ~ %s]",
                    !factoryType ? '*' : '0' + factoryType,
                    summary.index,
                    getPerfPosStr(static_cast<PerformancePos>(prevPos)),
                    getPerfPosStr(static_cast<PerformancePos>(pos)));
            break;
        case TM_INTERVAL:
            /* show fps */
            ret = prefixLogStr.appendFormat("[FACTORY(%c)/%d.%s]",
                    !factoryType ? '*' : '0' + factoryType,
                    summary.index,
                    getPerfPosStr(static_cast<PerformancePos>(pos)));
            break;
        default:
            break;
        }

        // main log
        mainLogStr.appendFormat("%s", prefixLogStr.c_str());
        nsecs_t avg = summary.time.sum / summary.time.count;

        switch (tType) {
        case TM_DURATION:
            ret = mainLogStr.append("= DURATION | ");
            break;
        case TM_INTERVAL:
            {
                double fps = 1000000.0f / avg;
                ret = mainLogStr.appendFormat("= INTERVAL(%.2f fps) | ", fps);
            }
        default:
            break;
        }

        ret = mainLogStr.appendFormat("%ju us, %ju us, %ju us | err %d",
                avg, summary.time.min, summary.time.max, summary.errCnt);

        // print main log
        m_printLog(prio, tag, mainLogStr);

        // sub log (pipe)
        TimeSubInfo_sp_t sub = summary.sub;
        std::map<int32_t, TimeUnitInfo> *duration = &sub->duration;
        std::map<int32_t, TimeUnitInfo> *interval = &sub->interval;

        if (duration->size() > 0) {
            String8 subDurationStr;
            String8 subIntervalStr;

            std::map<int32_t, TimeUnitInfo>::iterator it;

            subDurationStr.appendFormat("%s", prefixLogStr.c_str());
            subDurationStr.append("=== PIPE_DURATION ");

            it = duration->begin();
            for (; it != duration->end(); ++it) {
                subDurationStr.appendFormat("[P%d | %ju,%ju,%ju]-> ",
                        it->first,
                        ((it->second).sum / (it->second).count),
                        (it->second).min,
                        (it->second).max);
            }

            // print sub log - pipe's duration
            m_printLog(prio, tag, subDurationStr);

            subIntervalStr.appendFormat("%s", prefixLogStr.c_str());
            subIntervalStr.append("=== PIPE_INTERVAL ");

            it = interval->begin();
            for (; it != interval->end(); ++it) {
                nsecs_t avg = ((it->second).sum / (it->second).count);
                double fps = 1000000.0f / avg;
                subIntervalStr.appendFormat("[P%d(%.2f fps) | %ju,%ju,%ju]-> ",
                        it->first, fps, avg,
                        (it->second).min,
                        (it->second).max);
            }

            // print sub log - pipe's interval
            m_printLog(prio, tag, subIntervalStr);
        }

        TimeInfo empty;
        empty.index = summary.index;
        m_perfSummary[pType][summaryKey] = empty;
    }
}
