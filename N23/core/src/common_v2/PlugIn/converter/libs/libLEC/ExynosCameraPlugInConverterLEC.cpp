/*
 * Copyright (C) 2018, Samsung Electronics Co. LTD
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

//#define LOG_NDEBUG 0
#define LOG_TAG "ExynosCameraPlugInConverterLEC"

#include "ExynosCameraPlugInConverterLEC.h"

namespace android {

/*********************************************/
/*  protected functions                      */
/*********************************************/
status_t ExynosCameraPlugInConverterLEC::m_init(void)
{
    strncpy(m_name, "ConverterLEC", (PLUGIN_NAME_STR_SIZE - 1));

    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterLEC::m_deinit(void)
{
    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterLEC::m_create(__unused Map_t *map)
{
    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterLEC::m_setup(__unused Map_t *map)
{
    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterLEC::m_make(Map_t *map)
{
    status_t ret = NO_ERROR;
    enum PLUGIN_CONVERT_TYPE_T convertType;
    ExynosCameraFrameSP_sptr_t frame = NULL;
    ExynosCameraParameters *parameters = NULL;

    convertType = (enum PLUGIN_CONVERT_TYPE_T)(unsigned long)(*map)[PLUGIN_CONVERT_TYPE];
    frame = (ExynosCameraFrame *)(*map)[PLUGIN_CONVERT_FRAME];
    parameters = (ExynosCameraParameters *)(*map)[PLUGIN_CONVERT_PARAMETER];

    switch (convertType) {
    case PLUGIN_CONVERT_PROCESS_BEFORE:
        (*map)[PLUGIN_OPER_MODE] = (Map_data_t)1;
        (*map)[PLUGIN_RESULT] = (Map_data_t)0;
        break;
    case PLUGIN_CONVERT_PROCESS_AFTER:
    {
        int result = (*map)[PLUGIN_RESULT];
        break;
    }
    default:
        CLOGE("Not supported convert type(%d)", convertType);
        break;
    }

func_exit:
    return ret;
}
}; /* namespace android */
