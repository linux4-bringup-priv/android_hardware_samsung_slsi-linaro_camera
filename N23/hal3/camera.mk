#
# Copyright (C) 2013 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# camera external plugIn
PRODUCT_PACKAGES += \
	libexynoscamera_plugin_utils
ifeq ($(BOARD_CAMERA_USES_LLS_SOLUTION), true)
PRODUCT_PACKAGES += \
	libexynoscamera_lowlightshot_plugin
endif
ifeq ($(BOARD_CAMERA_USES_DUAL_CAMERA_SOLUTION_FAKE), true)
PRODUCT_PACKAGES += \
	libexynoscamera_fakefusion_plugin
endif
ifeq ($(BOARD_CAMERA_USES_DUAL_CAMERA_SOLUTION_ARCSOFT), true)
PRODUCT_PACKAGES += \
	libexynoscamera_arcsoftfusion_plugin
endif
ifeq ($(BOARD_CAMERA_USES_CAMERA_SOLUTION_VDIS), true)
PRODUCT_PACKAGES += \
	libexynoscamera_vdis_plugin
endif

ifeq ($(BOARD_CAMERA_USES_SENSOR_GYRO_FACTORY_MODE), true)
PRODUCT_PACKAGES += \
	libGyroST
endif

ifeq ($(BOARD_CAMERA_USES_COMBINE_PLUGIN), true)
PRODUCT_PACKAGES += \
       libexynoscamera_combine_preview_plugin \
       libexynoscamera_combine_reprocessing_plugin
endif

ifeq ($(BOARD_CAMERA_USES_HIFI_LLS_CAPTURE), true)
PRODUCT_PACKAGES += \
	libexynoscamera_hifills_plugin
endif

ifeq ($(BOARD_CAMERA_USES_HIFI_CAPTURE), true)
PRODUCT_PACKAGES += \
	libexynoscamera_hifi_plugin
endif

ifeq ($(BOARD_CAMERA_USES_EXYNOS_GDC), true)
PRODUCT_PACKAGES += \
       libexynoscamera_exynosgdc_plugin
endif
ifeq ($(BOARD_CAMERA_USES_EXYNOS_VPL), true)
PRODUCT_PACKAGES += \
	libexynoscamera_vpl_plugin \
	libvpl \
	libopencv_core \
	libopencv_imgproc
endif
ifeq ($(BOARD_CAMERA_USES_EXYNOS_LEC), true)
PRODUCT_PACKAGES += \
	libexynoscamera_exynoslec_plugin
endif

