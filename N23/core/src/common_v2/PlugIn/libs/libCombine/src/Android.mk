# Copyright 2019 The Android Open Source Project
#
# $(warning #############################################)
# $(warning ######## Combine Preview Fake library #######)
# $(warning #############################################)

ifeq ($(BOARD_CAMERA_BUILD_SOLUTION_SRC), true)
LOCAL_PATH:= $(call my-dir)
CAMERA_PATH := $(TOP)/vendor/samsung_slsi/exynos/camera/N23
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_PROPRIETARY_MODULE := true

LOCAL_SHARED_LIBRARIES := liblog libutils libcutils libexynosutils

LOCAL_SRC_FILES:= \
	FakeSceneDetect.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../include \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(CAMERA_PATH)/core/src/common_v2/PlugIn/include \

LOCAL_MODULE := libFakeSceneDetect

LOCAL_CFLAGS += -Wno-error=date-time
LOCAL_CFLAGS := -Wno-unused-parameter
LOCAL_CFLAGS += -Wno-date-time
LOCAL_CFLAGS += -Wno-unused-variable

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_SHARED_LIBRARY)
endif
