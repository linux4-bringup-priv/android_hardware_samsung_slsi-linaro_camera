/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#ifndef FAKE_SCENE_DETECT_H
#define FAKE_SCENE_DETECT_H

#include <utils/Log.h>
#include <utils/threads.h>
#include <utils/Timers.h>
#include "string.h"

#include "exynos_format.h"
#include "PlugInCommon.h"
#include "ExynosCameraPlugInUtils.h"

using namespace android;

/*
 * \ingroup ExynosCamera
*/
class FakeSceneDetect
{
public:
    FakeSceneDetect();
    virtual ~FakeSceneDetect();

    virtual status_t create(void);
    virtual status_t destroy(void);
    virtual status_t start(void);
    virtual status_t stop(void);
    virtual status_t setup(Map_t *map);
    virtual status_t execute(Map_t *map);
    virtual status_t init(Map_t *map);
    virtual status_t query(Map_t *map);
/* Library APIs */
private:
    int     m_sceneType;
};

#endif //FAKE_SCENE_DETECT_H

