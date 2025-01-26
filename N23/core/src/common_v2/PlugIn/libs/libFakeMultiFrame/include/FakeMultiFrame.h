/*
 * Copyright @ 2019, Samsung Electronics Co. LTD
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
 * \file      FakeMultiFrame.h
 * \brief     header file for FakeMultiFrame
 * \author    Sangwoo, Park(sw5771.park@samsung.com)
 * \date      2019/04/26
 *
 * <b>Revision History: </b>
 * - 2019/04/26 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Initial version
 */

#ifndef FAKE_MULTI_FRAME_H
#define FAKE_MULTI_FRAME_H

#include <utils/Log.h>
#include <utils/threads.h>
#include <utils/Timers.h>
#include "string.h"

#include <linux/videodev2.h>
#include <linux/videodev2_exynos_media.h>

#include "exynos_format.h"
#include "PlugInCommon.h"

using namespace android;

#define FUSION_PROCESSTIME_STANDARD (34000)

//! FakeMultiFrame
/*!
 * \ingroup ExynosCamera
 */
class FakeMultiFrame
{
public:
    //! Constructor
    FakeMultiFrame();
    //! Destructor
    virtual ~FakeMultiFrame();

    //! create
    virtual status_t create(void);

    //! destroy
    virtual status_t  destroy(void);

    //! execute
    virtual status_t  execute(Map_t *map);

    //! init
    virtual status_t  init(Map_t *map);

    //! query
    virtual status_t  query(Map_t *map);

protected:
    void              m_copyMetaData(char *srcMetaAddr, int srcMetaSize, char *dstMetaAddr, int dstMetaSize);
    char              m_v4l2Format2Char(int v4l2Format, int pos);

protected:
    int               m_emulationProcessTime;
    float             m_emulationCopyRatio;
};

#endif //FAKE_MULTI_FRAME_H
