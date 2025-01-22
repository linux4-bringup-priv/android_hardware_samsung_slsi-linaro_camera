# Copyright 2017 The Android Open Source Project

ifeq ($(BOARD_CAMERA_USES_SLSI_PLUGIN_N23), true)

LOCAL_PATH := $(call my-dir)
CAMERA_PATH := $(TOP)/vendor/samsung_slsi/exynos/camera/N23
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ExynosCameraPlugIn.cpp
LOCAL_SHARED_LIBRARIES := libutils libcutils liblog

LOCAL_MODULE := libexynoscamera_plugin

LOCAL_C_INCLUDES += \
    $(CAMERA_PATH)/core/src/common_v2 \
    $(CAMERA_PATH)/core/src/common_v2/include \
    $(CAMERA_PATH)/core/src/common_v2/PlugIn \
    $(CAMERA_PATH)/core/src/common_v2/PlugIn/include \
    $(TOP)/bionic \
    $(TOP)/frameworks/native/libs/binder/include

LOCAL_CFLAGS := -Wno-unused-parameter
LOCAL_CFLAGS += -Wno-error=date-time
LOCAL_CFLAGS += -Wno-overloaded-virtual
LOCAL_CFLAGS += -Wno-unused-variable

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := ExynosCameraPlugInUtils.cpp
LOCAL_SHARED_LIBRARIES := libutils libcutils liblog libion

LOCAL_MODULE := libexynoscamera_plugin_utils

LOCAL_C_INCLUDES += \
    $(CAMERA_PATH)/core/src/common_v2 \
    $(CAMERA_PATH)/core/src/common_v2/include \
    $(CAMERA_PATH)/core/src/common_v2/PlugIn \
    $(CAMERA_PATH)/core/src/common_v2/PlugIn/include \
    $(TOP)/system/core/libion/include \
    $(TOP)/bionic \
    $(TOP)/frameworks/native/libs/binder/include

LOCAL_CFLAGS := -Wno-unused-parameter
LOCAL_CFLAGS += -Wno-error=date-time
LOCAL_CFLAGS += -Wno-overloaded-virtual

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_SHARED_LIBRARY)

# external plugins
include $(LOCAL_PATH)/libs/Android.mk

endif
