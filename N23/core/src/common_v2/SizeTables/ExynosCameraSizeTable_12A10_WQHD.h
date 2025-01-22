/*
**
** Copyright 2013, Samsung Electronics Co. LTD
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

#ifndef EXYNOS_CAMERA_LUT_12A10_H
#define EXYNOS_CAMERA_LUT_12A10_H

#include "ExynosCameraConfig.h"

/* -------------------------
    SIZE_RATIO_16_9 = 0,
    SIZE_RATIO_4_3,
    SIZE_RATIO_1_1,
    SIZE_RATIO_3_2,
    SIZE_RATIO_5_4,
    SIZE_RATIO_5_3,
    SIZE_RATIO_11_9,
    SIZE_RATIO_18P5_9,
    SIZE_RATIO_END
----------------------------
    RATIO_ID,
    SENSOR_W   = 1,
    SENSOR_H,
    BNS_W,
    BNS_H,
    BCROP_W,
    BCROP_H,
    BDS_W,
    BDS_H,
    TARGET_W,
    TARGET_H,
-----------------------------
    Sensor Margin Width  = 0,
    Sensor Margin Height = 0
-----------------------------*/

static int PREVIEW_SIZE_LUT_12A10[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1440p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 21:9 (Single) */
    { SIZE_RATIO_21_9,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* Dummy (not used) */
    { SIZE_RATIO_9_16,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    }
};

static int PICTURE_SIZE_LUT_12A10[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (4096 + 0) ,(3072 + 0) ,   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      4096      , 3072      ,   /* [bds    ] */
      4096      , 3072      ,   /* [target ] */
    },
    /* 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (4096 + 0) ,(3072 + 0) ,   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      4096      , 3072      ,   /* [bds    ] */
      4096      , 3072      ,   /* [target ] */
    },
    /* 21:9 (Single) */
    { SIZE_RATIO_21_9,
     (4096 + 0) ,(3072 + 0) ,   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      4096      , 3072      ,   /* [bds    ] */
      4096      , 3072      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4096 + 0) ,(3072 + 0) ,   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      4096      , 3072      ,   /* [bds    ] */
      4096      , 3072      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (4096 + 0) ,(3072 + 0) ,   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      4096      , 3072      ,   /* [bds    ] */
      4096      , 3072      ,   /* [target ] */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (4096 + 0) ,(3072 + 0) ,   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      4096      , 3072      ,   /* [bds    ] */
      4096      , 3072      ,   /* [target ] */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (4096 + 0) ,(3072 + 0) ,   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      4096      , 3072      ,   /* [bds    ] */
      4096      , 3072      ,   /* [target ] */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (4096 + 0) ,(3072 + 0) ,   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      4096      , 3072      ,   /* [bds    ] */
      4096      , 3072      ,   /* [target ] */
    },
    /* Dummy (not used) */
    { SIZE_RATIO_9_16,
     (4096 + 0) ,(3072 + 0) ,   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      4096      , 3072      ,   /* [bds    ] */
      4096      , 3072      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_12A10[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_21_9,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* Dummy (not used) */
    { SIZE_RATIO_9_16,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_12A10[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */
    /* FHD_60 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (2048 + 0) , (1536 + 0) ,   /* [sensor ] */
      2048      ,  1536      ,   /* [bns    ] */
      2048      ,  1536      ,   /* [bcrop  ] */
      2048      ,  1536      ,   /* [bds    ] */
      1920      ,  1080      ,   /* [target ] */
    },
};

static int VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_12A10[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */

    /* FHD_120 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (1280 + 0) , (720 + 0) ,   /* [sensor ] */
      1280      ,  720      ,   /* [bns    ] */
      1280      ,  720      ,   /* [bcrop  ] */
      1280      ,  720      ,   /* [bds    ] */
      1280      ,  720      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_12A10[][SIZE_OF_LUT] =
{
};

static int VTCALL_SIZE_LUT_12A10[][SIZE_OF_LUT] =
{
    /* Binning   = 2
       BNS ratio = 1.0
       BDS       = ON */

    /* 16:9 (VT_Call) */
    { SIZE_RATIO_16_9,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      3072      , 1728      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /* 4:3 (VT_Call) */
    { SIZE_RATIO_4_3,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      3072      , 2304      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (VT_Call) */
    { SIZE_RATIO_1_1,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2304      , 2304      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 3:2 (VT_Call) */
    { SIZE_RATIO_3_2,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      3072      , 2048      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    /* 11:9 (VT_Call) */
    { SIZE_RATIO_11_9,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2816      , 2304      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    }
};

static int FAST_AE_STABLE_SIZE_LUT_12A10[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 4.0 / FPS = 120
       BDS       = ON */

    /* HD_120 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (1280 + 0) , (720 + 0) ,   /* [sensor ] */
      1280      ,  720      ,   /* [bns    ] */
      1280      ,  720      ,   /* [bcrop  ] */
      1280      ,  720      ,   /* [bds    ] */
      1280      ,  720      ,   /* [target ] */
    },
};

static int PREVIEW_FULL_SIZE_LUT_12A10[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 21:9 (Single) */
    { SIZE_RATIO_21_9,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* Dummy (not used) */
    { SIZE_RATIO_9_16,
     (4096 + 0) ,(3072 + 0),   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    }
};

static int PICTURE_FULL_SIZE_LUT_12A10[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (4096 + 0) ,(3072 + 0) ,   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      4096      , 3072      ,   /* [bds    ] */
      4096      , 3072      ,   /* [target ] */
    },
    /* 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (4096 + 0) ,(3072 + 0) ,   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      4096      , 3072      ,   /* [bds    ] */
      4096      , 3072      ,   /* [target ] */
    },
    /* 21:9 (Single) */
    { SIZE_RATIO_21_9,
     (4096 + 0) ,(3072 + 0) ,   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      4096      , 3072      ,   /* [bds    ] */
      4096      , 3072      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4096 + 0) ,(3072 + 0) ,   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      4096      , 3072      ,   /* [bds    ] */
      4096      , 3072      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (4096 + 0) ,(3072 + 0) ,   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      4096      , 3072      ,   /* [bds    ] */
      4096      , 3072      ,   /* [target ] */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (4096 + 0) ,(3072 + 0) ,   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      4096      , 3072      ,   /* [bds    ] */
      4096      , 3072      ,   /* [target ] */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (4096 + 0) ,(3072 + 0) ,   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      4096      , 3072      ,   /* [bds    ] */
      4096      , 3072      ,   /* [target ] */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (4096 + 0) ,(3072 + 0) ,   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      4096      , 3072      ,   /* [bds    ] */
      4096      , 3072      ,   /* [target ] */
    },
    /* Dummy (not used) */
    { SIZE_RATIO_9_16,
     (4096 + 0) ,(3072 + 0) ,   /* [sensor ] */
      4096      , 3072      ,   /* [bns    ] */
      4096      , 3072      ,   /* [bcrop  ] */
      4096      , 3072      ,   /* [bds    ] */
      4096      , 3072      ,   /* [target ] */
    }
};

/* yuv stream size list */
static int OV12A10_YUV_LIST[][SIZE_OF_RESOLUTION] =
{
    /* { width, height, minFrameDuration, ratioId } */
    { 4096, 3072, 33331760, SIZE_RATIO_4_3},
    { 4096, 2304, 33331760, SIZE_RATIO_16_9},
    { 4096, 1760, 33331760, SIZE_RATIO_21_9},
    { 3840, 2160, 33331760, SIZE_RATIO_16_9},
    { 3840, 1632, 33331760, SIZE_RATIO_21_9},
    { 3264, 2448, 33331760, SIZE_RATIO_4_3},
    { 3264, 1836, 33331760, SIZE_RATIO_16_9},
    { 3264, 1400, 33331760, SIZE_RATIO_21_9},
    { 1920, 1080, 16665880, SIZE_RATIO_16_9},
    { 1920, 816, 33331760, SIZE_RATIO_21_9},
    { 1440, 1080, 33331760, SIZE_RATIO_4_3},
    { 1280,  960, 33331760, SIZE_RATIO_4_3},
    { 1280,  720, 33331760, SIZE_RATIO_16_9},
    {  960,  720, 33331760, SIZE_RATIO_4_3},
    {  800,  450, 33331760, SIZE_RATIO_16_9},
    {  720,  480, 33331760, SIZE_RATIO_3_2},
    {  640,  480, 33331760, SIZE_RATIO_4_3},
    {  352,  288, 33331760, SIZE_RATIO_11_9},
    {  320,  240, 33331760, SIZE_RATIO_4_3},
    {  256,  144, 33331760, SIZE_RATIO_16_9}, /* DngCreatorTest */
    {  176,  144, 33331760, SIZE_RATIO_11_9}, /* RecordingTest */
};

/* yuv reprocessing input stream size list */
static int OV12A10_YUV_REPROCESSING_INPUT_LIST[][SIZE_OF_RESOLUTION] =
{
    /* { width, height, minFrameDuration, ratioId } */
    { 4096, 3072, 33331760, SIZE_RATIO_4_3},
};

/* yuv reprocessing input stream size list */
static int OV12A10_RAW_OUTPUT_LIST[][SIZE_OF_RESOLUTION] =
{
    /* { width, height, minFrameDuration, ratioId } */
    { 4096, 3072, 33331760, SIZE_RATIO_4_3},
};

/* availble Jpeg size (only for  HAL_PIXEL_FORMAT_BLOB) */
static int OV12A10_JPEG_LIST[][SIZE_OF_RESOLUTION] =
{
    /* { width, height, minFrameDuration, ratioId } */
    { 4096, 3072, 33331760, SIZE_RATIO_4_3},
    { 4096, 2304, 33331760, SIZE_RATIO_16_9},
    { 4096, 1760, 33331760, SIZE_RATIO_21_9},
    { 3840, 2160, 33331760, SIZE_RATIO_16_9},
    { 3840, 1632, 33331760, SIZE_RATIO_21_9},
    { 3264, 2448, 33331760, SIZE_RATIO_4_3},
    { 3264, 1836, 33331760, SIZE_RATIO_16_9},
    { 3264, 1400, 33331760, SIZE_RATIO_21_9},
    { 1920, 1080, 33331760, SIZE_RATIO_16_9},
    { 1920, 816, 33331760, SIZE_RATIO_21_9},
    { 1440, 1080, 33331760, SIZE_RATIO_4_3},
    { 1280,  960, 33331760, SIZE_RATIO_4_3},
    { 1280,  720, 33331760, SIZE_RATIO_16_9},
    {  960,  720, 33331760, SIZE_RATIO_4_3},
    {  800,  450, 33331760, SIZE_RATIO_16_9},
    {  720,  480, 33331760, SIZE_RATIO_3_2},
    {  640,  480, 33331760, SIZE_RATIO_4_3},
    {  352,  288, 33331760, SIZE_RATIO_11_9},
    {  320,  240, 33331760, SIZE_RATIO_4_3},
};

static int OV12A10_THUMBNAIL_LIST[][SIZE_OF_RESOLUTION] =
{
    {  512,  384, 0, SIZE_RATIO_4_3},
    {  512,  288, 0, SIZE_RATIO_16_9},
    {  384,  384, 0, SIZE_RATIO_1_1},
    {  504,  216, 0, SIZE_RATIO_21_9},
    {    0,    0, 0, SIZE_RATIO_1_1},
};

/* For HAL3 */
static int OV12A10_HIGH_SPEED_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1280,   720, 120, SIZE_RATIO_16_9},
};

static int OV12A10_FPS_RANGE_LIST[][2] =
{
    {   7000,  15000},
    {  15000,  15000},
    {   7000,  20000},
    {   7000,  24000},
    {  10000,  24000},
    {  24000,  24000},
    {   7000,  30000},
    {  10000,  30000},
    {  15000,  30000},
    {  24000,  30000},
    {  30000,  30000},
};

static int OV12A10_HIDDEN_FPS_RANGE_LIST[][2] =
{
    {  60000,  60000},
};

/* For HAL3 */
static int OV12A10_HIGH_SPEED_VIDEO_FPS_RANGE_LIST[][2] =
{
    { 30000, 120000},
    { 120000, 120000},
};

/* vendor static info : width, height, min_fps, max_fps, vdis width, vdis height, recording limit time(sec) */
static int OV12A10_AVAILABLE_VIDEO_LIST[][7] =
{
    { 3840, 2160, 30000, 30000, 4032, 2268, 600},
    { 3840, 1632, 30000, 30000, 4032, 1714, 600},
    { 1920, 1080, 60000, 60000, 2304, 1296, 600},
    { 1920, 1080, 30000, 30000, 2304, 1296, 0},
    { 1920,  816, 30000, 30000, 2304, 980, 0},
    { 1280,  720, 30000, 30000, 1536, 864, 0},
    {  640,  480, 30000, 30000, 0, 0, 0},
    {  320,  240, 30000, 30000, 0, 0, 0},    /* For support the CameraProvider lib of Message app*/
    {  176,  144, 30000, 30000, 0, 0, 0},    /* For support the CameraProvider lib of Message app*/
};

/*  vendor static info :  width, height, min_fps, max_fps, recording limit time(sec) */
static int OV12A10_AVAILABLE_HIGH_SPEED_VIDEO_LIST[][5] =
{
    { 1280, 720, 120000, 120000, 0},
};

/* effect fps range */
static int OV12A10_EFFECT_FPS_RANGE_LIST[][2] =
{
    {  10000,  24000},
    {  24000,  24000},
};

static camera_metadata_rational COLOR_MATRIX1_OV12A10_3X3[] =
{
    {661, 1024}, {-62, 1024}, {-110, 1024},
    {-564, 1024}, {1477, 1024}, {77, 1024},
    {-184, 1024}, {445, 1024}, {495, 1024}
};

static camera_metadata_rational COLOR_MATRIX2_OV12A10_3X3[] =
{
    {1207, 1024}, {-455, 1024}, {-172, 1024},
    {-488, 1024}, {1522, 1024}, {107, 1024},
    {-82, 1024}, {314, 1024}, {713, 1024}
};

static camera_metadata_rational FORWARD_MATRIX1_OV12A10_3X3[] =
{
    {759, 1024}, {5, 1024}, {223, 1024},
    {292, 1024}, {732, 1024}, {0, 1024},
    {13, 1024}, {-494, 1024}, {1325, 1024}
};

static camera_metadata_rational FORWARD_MATRIX2_OV12A10_3X3[] =
{
    {655, 1024}, {68, 1024}, {265, 1024},
    {186, 1024}, {810, 1024}, {28, 1024},
    {-34, 1024}, {-821, 1024}, {1700, 1024}
};
#endif
