# Copyright (C) 2018 The Android Open Source Project
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

$(warning #############################################)
$(warning ########       LEC PlugIn       #############)
$(warning #############################################)

LOCAL_PATH := $(call my-dir)
CAMERA_PATH := $(TOP)/vendor/samsung_slsi/exynos/camera/N23/v1

# builtin addbayer lib
ifeq ($(TARGET_2ND_ARCH),)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := true
LOCAL_PREBUILT_LIBS := lib32/libaddbayer.so

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_MULTI_PREBUILT)

else
include $(CLEAR_VARS)

LOCAL_MODULE := libaddbayer
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES_$(TARGET_ARCH) := lib64/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_SRC_FILES_$(TARGET_2ND_ARCH) := lib32/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_MULTILIB := both

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_PREBUILT)
endif

####################
# exynosgdc plugin
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ExynosCameraPlugInLEC.cpp
LOCAL_SHARED_LIBRARIES := libutils libcutils liblog libexynoscamera_plugin libexynosutils libaddbayer

LOCAL_MODULE := libexynoscamera_exynoslec_plugin

LOCAL_C_INCLUDES += \
	$(TOP)/system/core/libcutils/include \
	$(TOP)/hardware/samsung_slsi/graphics/base/libion/include \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(CAMERA_PATH)/core/src/common_v2/ \
	$(CAMERA_PATH)/core/src/common_v2/PlugIn/ \
	$(CAMERA_PATH)/core/src/common_v2/PlugIn/include \
	$(CAMERA_PATH)/core/src/common_v2/PlugIn/libs/include \
	$(CAMERA_PATH)/core/src/common_v2/PlugIn/libs/libLEC/include

LOCAL_CFLAGS := -Wno-unused-parameter
LOCAL_CFLAGS += -Wno-error=date-time

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_SHARED_LIBRARY)

# build sources to make prebuilt exynoslec lib
# include $(LOCAL_PATH)/src/Android.mk

