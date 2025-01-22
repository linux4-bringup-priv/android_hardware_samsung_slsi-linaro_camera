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

/*#define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraPlugInLEC"
#include <log/log.h>

#include <hardware/exynos/ion.h>
#include "ExynosCameraPlugInLEC.h"
#include "addbayer.h"
using namespace lec;

namespace android {

volatile int32_t ExynosCameraPlugInLEC::initCount = 0;

DECLARE_CREATE_PLUGIN_SYMBOL(ExynosCameraPlugInLEC);

/*********************************************/
/*  protected functions                      */
/*********************************************/
status_t ExynosCameraPlugInLEC::m_init(void)
{
    int count = android_atomic_inc(&initCount);

    CLOGD("count(%d)", count);

    if (count == 1) {
        /* do nothing */
    }

    return NO_ERROR;
}

status_t ExynosCameraPlugInLEC::m_deinit(void)
{
    int count = android_atomic_dec(&initCount);

    CLOGD("count(%d)", count);

    if (count == 0) {
        /* do nothing */
    }

    return NO_ERROR;
}

status_t ExynosCameraPlugInLEC::m_create(void)
{
    CLOGD("");

    return NO_ERROR;
}

status_t ExynosCameraPlugInLEC::m_destroy(void)
{
    CLOGD("");

    return NO_ERROR;
}

status_t ExynosCameraPlugInLEC::m_setup(__unused Map_t *map)
{
    CLOGD("");

    return NO_ERROR;
}

status_t ExynosCameraPlugInLEC::m_process(Map_t *map)
{
    CLOGD("ADDBAYER process start");
    status_t ret = NO_ERROR;

    Array_buf_addr_t src_buf_addr = (Array_buf_addr_t)(*map)[PLUGIN_SRC_BUF_1];
    Array_buf_addr_t dst_buf_addr = (Array_buf_addr_t)(*map)[PLUGIN_DST_BUF_1];
    Array_buf_rect_t src_buf_pixel = (Array_buf_rect_t)(*map)[PLUGIN_SRC_BUF_RECT];
    Array_buf_rect_t dst_buf_pixel = (Array_buf_rect_t)(*map)[PLUGIN_DST_BUF_RECT];
    Array_buf_t src_v4l2_format = (Array_buf_t)(*map)[PLUGIN_SRC_BUF_V4L2_FORMAT];
    int oper_mode = (*map)[PLUGIN_OPER_MODE];

    char *psrc = src_buf_addr[0];
    char *pdst = dst_buf_addr[0];
    unsigned int copypixel = src_buf_pixel[0][PLUGIN_ARRAY_RECT_W] * src_buf_pixel[0][PLUGIN_ARRAY_RECT_H];
    if((src_buf_pixel[0][PLUGIN_ARRAY_RECT_W] != dst_buf_pixel[0][PLUGIN_ARRAY_RECT_W]) ||
            (src_buf_pixel[0][PLUGIN_ARRAY_RECT_H] != dst_buf_pixel[0][PLUGIN_ARRAY_RECT_H]))
        CLOGE("ADDBAYER : PLIGIN_SRC_BUF_SIZE is different form PLUGIN_DST_BUF_SIZE");

    if(oper_mode == 0) { // cpu
        if(src_v4l2_format[0] == V4L2_PIX_FMT_SBGGR10P) { // packed
            CLOGE("ADDBAYER CPU operation, 10-bit data");
            ret = addBayerBufferByCpuPacked(psrc, pdst, copypixel);
        }else if(src_v4l2_format[0] == V4L2_PIX_FMT_SBGGR10) { // unpacked
            CLOGE("ADDBAYER CPU operation, 16-bit data");
            ret = addBayerBufferByCpu(psrc, pdst, copypixel);
        }else{
            ret = BAD_VALUE; // error
        }
    }else if(oper_mode == 1) { // neon

        if(src_v4l2_format[0] == V4L2_PIX_FMT_SBGGR10P) { // packed
            CLOGE("ADDBAYER NEON operation, 10-bit data");
            ret = addBayerBufferByNeonPacked(psrc, pdst, copypixel);
        }else if(src_v4l2_format[0] == V4L2_PIX_FMT_SBGGR10) { // unpacked
            CLOGE("ADDBAYER NEON operation, 16-bit data");
            ret = addBayerBufferByNeon(psrc, pdst, copypixel);
        }else{
            ret = BAD_VALUE; // error
        }
    }else{
        ret = BAD_VALUE; // error
    }
    CLOGD("ADDBAYER process done. ret = %d",ret);

    return ret;
}

status_t ExynosCameraPlugInLEC::m_setParameter(int key, void *data)
{
    /* do nothing */
    return NO_ERROR;
}

status_t ExynosCameraPlugInLEC::m_getParameter(int key, void *data)
{
    /* do nothing */
    return NO_ERROR;
}

void ExynosCameraPlugInLEC::m_dump(void)
{
    /* do nothing */
}

status_t ExynosCameraPlugInLEC::m_query(Map_t *map)
{
    if (!map) {
        CLOGE("map is NULL");
        return BAD_VALUE;
    }

    (*map)[PLUGIN_VERSION]                = (Map_data_t) MAKE_VERSION(2, 1);
    (*map)[PLUGIN_LIB_NAME]               = (Map_data_t) "LibExynosLEC";
    (*map)[PLUGIN_BUILD_DATE]             = (Map_data_t) __DATE__;
    (*map)[PLUGIN_BUILD_TIME]             = (Map_data_t) __TIME__;
    (*map)[PLUGIN_PLUGIN_CUR_SRC_BUF_CNT] = (Map_data_t) 1;
    (*map)[PLUGIN_PLUGIN_CUR_DST_BUF_CNT] = (Map_data_t) 1;
    (*map)[PLUGIN_PLUGIN_MAX_SRC_BUF_CNT] = (Map_data_t) 1;
    (*map)[PLUGIN_PLUGIN_MAX_DST_BUF_CNT] = (Map_data_t) 1;

    return NO_ERROR;
}
}
