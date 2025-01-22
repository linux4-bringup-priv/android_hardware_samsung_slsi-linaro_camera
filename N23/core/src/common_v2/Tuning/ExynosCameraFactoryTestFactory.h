/*
**
** Copyright 2018, Samsung Electronics Co. LTD
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

/*!
 * \file      ExynosCameraFactoryTestFactory.h
 * \brief     header file for ExynosCameraFactoryTestFactory
 * \author    Sangwoo, Park(sw5771.park@samsung.com)
 * \date      2018/11/13
 *
 * <b>Revision History: </b>
 * - 2018/11/13 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Initial version
 */

#ifndef EXYNOS_CAMERA_FACTORY_TEST_FACTORY_H
#define EXYNOS_CAMERA_FACTORY_TEST_FACTORY_H

#include "ExynosCameraFactoryTest.h"

/*
 * Class ExynosCameraFactoryTestFactory
 *
 * This is adjusted "Abstract Factory design-pattern"
 */
class ExynosCameraFactoryTestFactory
{
protected:
    ExynosCameraFactoryTestFactory(){}
    virtual ~ExynosCameraFactoryTestFactory(){}

public:
    /*
     * Use this API to get the real object.
     */
    static ExynosCameraFactoryTest *newFactoryTest(int cameraId, const char *testName);
};

#endif //EXYNOS_CAMERA_FACTORY_TEST_FACTORY_H
