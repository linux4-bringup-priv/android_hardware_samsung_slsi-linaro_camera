# Copyright 2017 The Android Open Source Project

ifeq ($(BOARD_CAMERA_BUILD_SOLUTION_SRC), true)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := LowLightShot.cpp
LOCAL_SHARED_LIBRARIES := libutils libcutils liblog libexynosutils libexynoscamera_plugin_utils libexynosv4l2

LOCAL_MODULE := liblowlightshot

LOCAL_C_INCLUDES += \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(CAMERA_PATH)/core/src/9xxx \
	$(CAMERA_PATH)/core/src/common_v2/PlugIn/include \
	$(LOCAL_PATH)/../include \

LOCAL_CFLAGS := -Wno-unused-parameter
LOCAL_CFLAGS += -Wno-error=date-time
LOCAL_CFLAGS += -Wno-unused-variable

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_SHARED_LIBRARY)
endif
