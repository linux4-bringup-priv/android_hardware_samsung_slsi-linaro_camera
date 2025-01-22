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

#ifndef EXYNOS_CAMERA_SLSI_VENDOR_TAGS_INFO_DEFAULT_H
#define EXYNOS_CAMERA_SLSI_VENDOR_TAGS_INFO_DEFAULT_H


#include <system/camera_metadata.h>
#include <system/camera_vendor_tags.h>

namespace android {

const char *vendor_section_name[] {
    "com.exynos.control",
    "com.exynos.vendor.sensor",
    "com.exynos.vendor.envinfo",
    "com.exynos.vendor.control",
    "com.exynos.vendor.stats",
    "com.exynos.vendor.jpeg_encode_crop",
    "com.exynos.vendor.snapshot",
    "com.exynos.vendor.factory",
    "com.exynos.vendor.sensor_meta_data",
    "com.exynos.vendor.quadra_cfa",
    "com.exynos.vendor.beauty_face",
    "com.exynos.vendor.abortcapture",
    "com.exynos.vendor.chi.override",
    "com.exynos.vendor.offline_process",
};

}; // namespace android
#endif


