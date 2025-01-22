/*
 * Copyright (C) 2019, Samsung Electronics Co. LTD
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
#define LOG_TAG "ExynosCameraPlugInConverterCombine"

#include "ExynosCameraPlugInConverterCombine.h"

namespace android {

/*********************************************/
/*  public functions                         */
/*********************************************/
int ExynosCameraPlugInConverterCombine::getVendorTagValue(ExynosCameraFrameSP_sptr_t frame, int vendorTagName)
{
    int ret = 0;

    ////////////////////////////////////////////////
    // check vendor tag
    sp<ExynosCameraVendorMetaData> vendormeta = NULL;
    vendormeta = frame->getVendorMeta();

    if (vendormeta) {
        camera_metadata_entry_t entry;
        entry = vendormeta->find(vendorTagName);
        if (entry.count > 0) {
            ret = (int)entry.data.u8[0];
        }
    } else {
        ret = -1;
    }

    ////////////////////////////////////////////////

    return ret;
}

void ExynosCameraPlugInConverterCombine::setMapValue(Map_t *map, int tagName, int value)
{
    // set it
    (*map)[tagName] = (Map_data_t)value;
}

int ExynosCameraPlugInConverterCombine::getMapValue(Map_t *map, int tagName)
{
    return (int)(Data_int32_t)(*map)[tagName];
}

void ExynosCameraPlugInConverterCombine::resetScenario(Map_t *map)
{
    ExynosCameraPlugInConverterCombine::setMapValue(map, PLUGIN_SCENARIO, 0);
}

void ExynosCameraPlugInConverterCombine::setScenario(Map_t *map, int scenario)
{
    ExynosCameraPlugInConverterCombine::setMapValue(map, PLUGIN_SCENARIO, scenario);
}

int ExynosCameraPlugInConverterCombine::getScenario(Map_t *map)
{
    return ExynosCameraPlugInConverterCombine::getMapValue(map, PLUGIN_SCENARIO);
}

void ExynosCameraPlugInConverterCombine::setVendorScenario(Map_t *map, enum VENDOR_SCENARIO vendorScenario, bool on)
{
    ////////////////////////////////////////////////
    // get the current value.
    int scenario = 0;
    scenario = ExynosCameraPlugInConverterCombine::getScenario(map);

    if (on == true) {
        ////////////////////////////////////////////////
        // bit on
        scenario |= (1 << vendorScenario);
    } else {
        ////////////////////////////////////////////////
        // bit off
        scenario &= ~(1 << vendorScenario);
    }

    ////////////////////////////////////////////////
    // set it
    ExynosCameraPlugInConverterCombine::setScenario(map, scenario);

    ////////////////////////////////////////////////
}

bool ExynosCameraPlugInConverterCombine::getVendorScenario(Map_t *map, enum VENDOR_SCENARIO vendorScenario)
{
    ////////////////////////////////////////////////
    // get the current value.
    int scenario = 0;
    scenario = ExynosCameraPlugInConverterCombine::getScenario(map);

    bool flagOn = false;

    ////////////////////////////////////////////////
    // check the bit is 1
    if (scenario & (1 << vendorScenario)) {
        flagOn = true;
    }

    ////////////////////////////////////////////////

    return flagOn;
}

}; /* namespace android */
