# Copyright 2017 The Android Open Source Project

$(warning #############################################)
$(warning ######## Sensor Gyro Self Test Lirary #######)
$(warning #############################################)

LOCAL_PATH := $(call my-dir)

# builtin libGyroST lib
ifeq ($(TARGET_2ND_ARCH),)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := true
LOCAL_PREBUILT_LIBS := lib32/libGyroST.so

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_MULTI_PREBUILT)

else
include $(CLEAR_VARS)

LOCAL_MODULE := libGyroST
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES_$(TARGET_ARCH) := lib64/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_SRC_FILES_$(TARGET_2ND_ARCH) := lib32/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_MULTILIB := both

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_PREBUILT)
endif
