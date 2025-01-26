# Copyright 2017 The Android Open Source Project

LOCAL_PATH := $(call my-dir)

# builtin fakefusion lib
ifeq ($(TARGET_2ND_ARCH),)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := true
#LOCAL_PROPRIETARY_MODULE := true

LOCAL_PREBUILT_LIBS := lib32/libexynoscamera_fakefusion.so

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_MULTI_PREBUILT)

else
include $(CLEAR_VARS)

LOCAL_MODULE := libexynoscamera_fakefusion
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional

#LOCAL_PRELINK_MODULE := false
#LOCAL_PROPRIETARY_MODULE := true

LOCAL_SRC_FILES_$(TARGET_ARCH) := lib64/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_SRC_FILES_$(TARGET_2ND_ARCH) := lib32/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
LOCAL_MULTILIB := both

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_PREBUILT)
endif

# fakefusion plugin
$(warning #############################################)
$(warning ########     fakefusion_plugin  #############)
$(warning #############################################)
include $(CLEAR_VARS)
CAMERA_PATH := $(TOP)/vendor/samsung_slsi/exynos/camera/N23

#LOCAL_PRELINK_MODULE := false
#LOCAL_PROPRIETARY_MODULE := true

LOCAL_SRC_FILES := ExynosCameraPlugInFakeFusion.cpp
LOCAL_SHARED_LIBRARIES := libutils libcutils liblog libexynoscamera_plugin libexynosutils libexynoscamera_fakefusion

LOCAL_MODULE := libexynoscamera_fakefusion_plugin

LOCAL_C_INCLUDES += \
	$(TOP)/hardware/samsung_slsi/exynos/include \
	$(CAMERA_PATH)/core/src/common_v2/ \
	$(CAMERA_PATH)/core/src/common_v2/PlugIn/ \
	$(CAMERA_PATH)/core/src/common_v2/PlugIn/include \
	$(CAMERA_PATH)/core/src/common_v2/PlugIn/libs/include \
	$(CAMERA_PATH)/core/src/common_v2/PlugIn/libs/libFakeFusion/include

LOCAL_CFLAGS := -Wno-unused-parameter
LOCAL_CFLAGS += -Wno-error=date-time
LOCAL_CFLAGS += -Wno-unused-variable

include $(TOP)/hardware/samsung_slsi/exynos/BoardConfigCFlags.mk
include $(BUILD_SHARED_LIBRARY)

# build sources to make builtin fakefusion lib
#include $(LOCAL_PATH)/src/Android.mk
