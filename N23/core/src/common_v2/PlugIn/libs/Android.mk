# Copyright 2017 The Android Open Source Project

$(warning #############################################)
$(warning ########       PlugIn           #############)
$(warning #############################################)

plugin_subdirs :=

plugin_subdirs += \
    libFakeMultiFrame

ifeq ($(BOARD_CAMERA_USES_DUAL_CAMERA_SOLUTION_FAKE), true)
plugin_subdirs += \
    libFakeFusion
endif

ifeq ($(BOARD_CAMERA_USES_DUAL_CAMERA_SOLUTION_ARCSOFT), true)
plugin_subdirs += \
    libArcsoftFusion
endif

ifeq ($(BOARD_CAMERA_USES_LLS_SOLUTION), true)
plugin_subdirs += \
    libLLS
endif

ifeq ($(BOARD_CAMERA_USES_CAMERA_SOLUTION_VDIS), true)
plugin_subdirs += \
    libVDIS
endif

ifeq ($(BOARD_CAMERA_USES_HIFI_LLS_CAPTURE), true)
plugin_subdirs += \
    libHiFiLLS
endif

ifeq ($(BOARD_CAMERA_USES_HIFI_CAPTURE), true)
plugin_subdirs += \
    libNoiseEdge
endif

ifeq ($(BOARD_CAMERA_USES_COMBINE_PLUGIN), true)
plugin_subdirs += \
    libCombine
endif

ifeq ($(BOARD_CAMERA_USES_EXYNOS_VPL), true)
plugin_subdirs += \
    libVPL
endif

ifeq ($(BOARD_CAMERA_USES_EXYNOS_LEC), true)
plugin_subdirs += \
    libLEC
endif

# external plugins
include $(call all-named-subdir-makefiles,$(plugin_subdirs))

