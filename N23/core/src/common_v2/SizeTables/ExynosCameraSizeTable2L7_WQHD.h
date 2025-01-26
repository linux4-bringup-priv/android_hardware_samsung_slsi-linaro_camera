/*
**
** Copyright 2017, Samsung Electronics Co. LTD
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

#ifndef EXYNOS_CAMERA_LUT_2L7_H
#define EXYNOS_CAMERA_LUT_2L7_H

#include "ExynosCameraConfig.h"

/* -------------------------
    SIZE_RATIO_16_9 = 0,
    SIZE_RATIO_4_3,
    SIZE_RATIO_1_1,
    SIZE_RATIO_3_2,
    SIZE_RATIO_5_4,
    SIZE_RATIO_5_3,
    SIZE_RATIO_11_9,
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

static int PREVIEW_SIZE_LUT_S5K2L7[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1440p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4032 + 0) ,(2272 + 0),   /* [sensor ] */
      4032      , 2272      ,   /* [bns    ] */
      4032      , 2268      ,   /* [bcrop  ] */
      2688      , 1512      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4032 + 0) ,(3024 + 0),   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4032      , 3024      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4032 + 0),(3024 + 0),   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      3024      , 3024      ,   /* [bcrop  ] */
      2016      , 2016      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4032      , 2688      ,   /* [bcrop  ] */
      2688      , 1792      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      3780      , 3024      ,   /* [bcrop  ] */
      2480      , 1984      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4030      , 2418      ,   /* [bcrop  ] */
      2640      , 1584      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      3696      , 3024      ,   /* [bcrop  ] */
      2464      , 2016      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] */
    }
};

static int PREVIEW_SIZE_LUT_S5K2L7_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.5
       BDS       = 1440p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4032 + 0) ,(2272 + 0),   /* [sensor ] */
      2688      , 1512      ,   /* [bns    ] */
      2688      , 1512      ,   /* [bcrop  ] */
      2688      , 1512      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4032 + 0) ,(3024 + 0),   /* [sensor ] */
      2688      , 2016      ,   /* [bns    ] */
      2688      , 2016      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4032 + 0),(3024 + 0),   /* [sensor ] */
      2016      , 2016      ,   /* [bns    ] */
      2016      , 2016      ,   /* [bcrop  ] */
      2016      , 2016      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      2688      , 1356      ,   /* [bns    ] */
      2688      , 1356      ,   /* [bcrop  ] */
      2688      , 1356      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      2688      , 1356      ,   /* [bns    ] */
      2530      , 1356      ,   /* [bcrop  ] */
      2530      , 1356      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      2688      , 1356      ,   /* [bns    ] */
      2688      , 1612      ,   /* [bcrop  ] */
      2688      , 1612      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      2688      , 1356      ,   /* [bns    ] */
      2464      , 1356      ,   /* [bcrop  ] */
      2464      , 1356      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] */
    }
};

static int PICTURE_SIZE_LUT_S5K2L7[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4032 + 0),(2272 + 0),   /* [sensor ] */
      4032      , 2272      ,   /* [bns    ] */
      4032      , 2268      ,   /* [bcrop  ] */
      4032      , 2268      ,   /* [bds    ] */
      4032      , 2268      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4032 + 0),(3024 + 0),   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4032      , 3024      ,   /* [bcrop  ] */
      4032      , 3024      ,   /* [bds    ] */
      4032      , 3024      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4032 + 0),(3024 + 0),   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      3024      , 3024      ,   /* [bcrop  ] */
      3024      , 3024      ,   /* [bds    ] */
      3024      , 3024      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_S5K2L7[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (4032 + 0) ,(2272 + 0) ,   /* [sensor ] */
      4032      , 2272      ,   /* [bns    ] */
      4032      , 2268      ,   /* [bcrop  ] */
      2688      , 1512      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      1920      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4032      , 3024      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      3024      , 3024      ,   /* [bcrop  ] */
      2016      , 2016      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4032      , 2688      ,   /* [bcrop  ] */
      2688      , 1792      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      3780      , 3024      ,   /* [bcrop  ] */
      2480      , 1984      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4030      , 2418      ,   /* [bcrop  ] */
      2640      , 1584      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      3696      , 3024      ,   /* [bcrop  ] */
      2464      , 2016      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_S5K2L7[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */

    /* HD_120 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (2016 + 0) ,(1136 + 0) ,   /* [sensor ] */
      2016      , 1136      ,   /* [bns    ] */
      2016      , 1134      ,   /* [bcrop  ] */
      1280      ,  720      ,   /* [bds    ] */
      1280      ,  720      ,   /* [target ] */
    },
    /* HD_120 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (2016 + 0) ,(1136 + 0) ,   /* [sensor ] */
      2016      , 1136      ,   /* [bns    ] */
      1504      , 1128      ,   /* [bcrop  ] */
       960      ,  720      ,   /* [bds    ] */
       960      ,  720      ,   /* [target ] */
    },
    /* HD_120 1:1 (Single) */
    { SIZE_RATIO_1_1,
     (2016 + 0) ,(1136 + 0) ,   /* [sensor ] */
      2016      , 1136      ,   /* [bns    ] */
      1120      , 1120      ,   /* [bcrop  ] */
       720      ,  720      ,   /* [bds    ] */
       720      ,  720      ,   /* [target ] */
    },
    /* HD_120 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (2016 + 0) ,(1136 + 0) ,   /* [sensor ] */
      2016      , 1136      ,   /* [bns    ] */
      1680      , 1120      ,   /* [bcrop  ] */
      1056      ,  704      ,   /* [bds    ] */
      1056      ,  704      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_S5K2L7[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */

    /* HD_240 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (2016 + 0) ,(1136 + 0) ,   /* [sensor ] */
      2016      , 1136      ,   /* [bns    ] */
      2016      , 1134      ,   /* [bcrop  ] */
      2016      , 1134      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      1280      ,  720      ,   /* [target ] */
    },
    /* HD_240 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (2016 + 0) ,(1136 + 0) ,   /* [sensor ] */
      2016      , 1136      ,   /* [bns    ] */
      1504      , 1128      ,   /* [bcrop  ] */
      1504      , 1128      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
       960      ,  720      ,   /* [target ] */
    },
    /* HD_240 1:1 (Single) */
    { SIZE_RATIO_1_1,
     (2016 + 0) ,(1136 + 0) ,   /* [sensor ] */
      2016      , 1136      ,   /* [bns    ] */
      1120      , 1120      ,   /* [bcrop  ] */
      1120      , 1120      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
       720      ,  720      ,   /* [target ] */
    },
    /* HD_240 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (2016 + 0) ,(1136 + 0) ,   /* [sensor ] */
      2016      , 1136      ,   /* [bns    ] */
      1680      , 1120      ,   /* [bcrop  ] */
      1680      , 1120      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      1056      ,  704      ,   /* [target ] */
    }
};

static int VTCALL_SIZE_LUT_S5K2L7[][SIZE_OF_LUT] =
{
    /* Binning   = 2
       BNS ratio = 1.0
       BDS       = ON */

    /* 16:9 (VT_Call) */
    { SIZE_RATIO_16_9,
     (2016 + 0) ,(1136 + 0) ,   /* [sensor ] */
      2016      , 1136      ,   /* [bns    ] */
      2016      , 1134      ,   /* [bcrop  ] */
      2016      , 1134      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /* 4:3 (VT_Call) */
    { SIZE_RATIO_4_3,
     (2016 + 0) ,(1136 + 0) ,   /* [sensor ] */
      2016      , 1136      ,   /* [bns    ] */
      1504      , 1128      ,   /* [bcrop  ] */
      1504      , 1128      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (VT_Call) */
    { SIZE_RATIO_1_1,
     (2016 + 0) ,(1136 + 0) ,   /* [sensor ] */
      2016      , 1136      ,   /* [bns    ] */
      1120      , 1120      ,   /* [bcrop  ] */
      1120      , 1120      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    /* 3:2 (VT_Call) */
    { SIZE_RATIO_3_2,
     (2016 + 0) ,(1136 + 0) ,   /* [sensor ] */
      2016      , 1136      ,   /* [bns    ] */
      1680      , 1120      ,   /* [bcrop  ] */
      1680      , 1120      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    /* 11:9 (VT_Call) */
    { SIZE_RATIO_11_9,
     (2016 + 0) ,(1136 + 0) ,   /* [sensor ] */
      2016      , 1136      ,   /* [bns    ] */
      1232      , 1008      ,   /* [bcrop  ] */
      1232      , 1008      ,   /* [bds    ] */
      1232      , 1008      ,   /* [target ] */
    }
};

static int FAST_AE_STABLE_SIZE_LUT_S5K2L7[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 4.0 / FPS = 120
       BDS       = ON */

    /* FAST_AE 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (1008 + 0) , (756 + 0) ,   /* [sensor ] */
      1008      ,  756      ,   /* [bns    ] */
      1008      ,  756      ,   /* [bcrop  ] */
      1008      ,  756      ,   /* [bds    ] */
      1008      ,  756      ,   /* [target ] */
    },
};

static int PREVIEW_FULL_SIZE_LUT_S5K2L7[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4032      , 3024      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4032      , 3024      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4032      , 3024      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4032      , 3024      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4032      , 3024      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4032      , 3024      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4032      , 3024      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
};

static int PICTURE_FULL_SIZE_LUT_S5K2L7[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4032      , 3024      ,   /* [bcrop  ] */
      4032      , 3024      ,   /* [bds    ] */
      4032      , 3024      ,   /* [target ] */
    },
    /* 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4032      , 3024      ,   /* [bcrop  ] */
      4032      , 3024      ,   /* [bds    ] */
      4032      , 3024      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4032      , 3024      ,   /* [bcrop  ] */
      4032      , 3024      ,   /* [bds    ] */
      4032      , 3024      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4032      , 3024      ,   /* [bcrop  ] */
      4032      , 3024      ,   /* [bds    ] */
      4032      , 3024      ,   /* [target ] */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4032      , 3024      ,   /* [bcrop  ] */
      4032      , 3024      ,   /* [bds    ] */
      4032      , 3024      ,   /* [target ] */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4032      , 3024      ,   /* [bcrop  ] */
      4032      , 3024      ,   /* [bds    ] */
      4032      , 3024      ,   /* [target ] */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      4032      , 3024      ,   /* [bcrop  ] */
      4032      , 3024      ,   /* [bds    ] */
      4032      , 3024      ,   /* [target ] */
    },
};

/* yuv stream size list */
static int S5K2L7_YUV_LIST[][SIZE_OF_RESOLUTION] =
{
    /* { width, height, minFrameDuration, ratioId } */
    { 4032, 3024, 33331760, SIZE_RATIO_4_3},
    { 4032, 2268, 33331760, SIZE_RATIO_16_9},
    { 3024, 3024, 33331760, SIZE_RATIO_1_1},
    { 3984, 2988, 33331760, SIZE_RATIO_4_3},
    { 3840, 2160, 16665880, SIZE_RATIO_16_9},
    { 3264, 2448, 16665880, SIZE_RATIO_4_3},
    { 3264, 1836, 16665880, SIZE_RATIO_16_9},
    { 2976, 2976, 16665880, SIZE_RATIO_1_1},
    { 2880, 2160, 16665880, SIZE_RATIO_4_3},
    { 2560, 1440, 16665880, SIZE_RATIO_16_9},
    { 2160, 2160, 16665880, SIZE_RATIO_1_1},
    { 2048, 1152, 16665880, SIZE_RATIO_16_9},
    { 1920, 1080, 16665880, SIZE_RATIO_16_9},
    { 1440, 1080, 16665880, SIZE_RATIO_4_3},
    { 1088, 1088, 16665880, SIZE_RATIO_1_1},
    { 1280,  720, 16665880, SIZE_RATIO_16_9},
    { 1056,  704, 16665880, SIZE_RATIO_3_2},
    { 1024,  768, 16665880, SIZE_RATIO_4_3},
    {  960,  720, 16665880, SIZE_RATIO_4_3},
    {  800,  450, 16665880, SIZE_RATIO_16_9},
    {  720,  720, 16665880, SIZE_RATIO_1_1},
    {  720,  480, 16665880, SIZE_RATIO_3_2},
    {  640,  480, 16665880, SIZE_RATIO_4_3},
    {  352,  288, 16665880, SIZE_RATIO_11_9},
    {  320,  240, 16665880, SIZE_RATIO_4_3},
    {  256,  144, 16665880, SIZE_RATIO_16_9}, /* DngCreatorTest */
    {  176,  144, 16665880, SIZE_RATIO_11_9}, /* RecordingTest */
};

/* availble Jpeg size (only for  HAL_PIXEL_FORMAT_BLOB) */
static int S5K2L7_JPEG_LIST[][SIZE_OF_RESOLUTION] =
{
    /* { width, height, minFrameDuration, ratioId } */
    { 4032, 3024, 50000000, SIZE_RATIO_4_3},
    { 4032, 2268, 50000000, SIZE_RATIO_16_9},
    { 3024, 3024, 50000000, SIZE_RATIO_1_1},
    { 3984, 2988, 50000000, SIZE_RATIO_4_3},
    { 3840, 2160, 50000000, SIZE_RATIO_16_9},
    { 3264, 2448, 50000000, SIZE_RATIO_4_3},
    { 3264, 1836, 50000000, SIZE_RATIO_16_9},
    { 2976, 2976, 50000000, SIZE_RATIO_1_1},
    { 2880, 2160, 50000000, SIZE_RATIO_4_3},
    { 2560, 1440, 50000000, SIZE_RATIO_16_9},
    { 2160, 2160, 50000000, SIZE_RATIO_1_1},
    { 2048, 1152, 50000000, SIZE_RATIO_16_9},
    { 1920, 1080, 33331760, SIZE_RATIO_16_9},
    { 1440, 1080, 33331760, SIZE_RATIO_4_3},
    { 1088, 1088, 33331760, SIZE_RATIO_1_1},
    { 1280,  720, 33331760, SIZE_RATIO_16_9},
    { 1056,  704, 33331760, SIZE_RATIO_3_2},
    { 1024,  768, 33331760, SIZE_RATIO_4_3},
    {  960,  720, 33331760, SIZE_RATIO_4_3},
    {  800,  450, 33331760, SIZE_RATIO_16_9},
    {  720,  720, 33331760, SIZE_RATIO_1_1},
    {  720,  480, 33331760, SIZE_RATIO_3_2},
    {  640,  480, 33331760, SIZE_RATIO_4_3},
    {  352,  288, 33331760, SIZE_RATIO_11_9},
    {  320,  240, 33331760, SIZE_RATIO_4_3},
};

/* For HAL3 */
static int S5K2L7_HIGH_SPEED_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1280,  720, 0, SIZE_RATIO_16_9},
};

static int S5K2L7_FPS_RANGE_LIST[][2] =
{
    {  15000,  15000},
    {  24000,  24000},
    {  7000,  30000},
    {  10000,  30000},
    {  15000,  30000},
    {  30000,  30000},
};

/* For HAL3 */
static int S5K2L7_HIGH_SPEED_VIDEO_FPS_RANGE_LIST[][2] =
{
    {  30000, 120000},
    { 120000, 120000},
    {  30000, 240000},
    { 240000, 240000},
};

/* vendor static info : width, height, min_fps, max_fps, vdis width, vdis height, recording limit time(sec) */
static int S5K2L7_VIDEO_LIST[][7] =
{
#ifdef USE_UHD_RECORDING
    { 3840, 2160, 30000, 30000, 0, 0, 600},
#endif
#ifdef USE_WQHD_RECORDING
    { 2560, 1440, 30000, 30000, 0, 0, 0},
#endif
    { 1920, 1080, 60000, 60000, 0, 0, 600},
    { 1920, 1080, 30000, 30000, 0, 0, 0},
    { 1440, 1440, 30000, 30000, 0, 0, 0},
    { 1280,  720, 30000, 30000, 0, 0, 0},
    {  640,  480, 30000, 30000, 0, 0, 0},
};

static camera_metadata_rational UNIT_MATRIX_S5K2L7_3X3[] =
{
    {128, 128}, {0, 128}, {0, 128},
    {0, 128}, {128, 128}, {0, 128},
    {0, 128}, {0, 128}, {128, 128}
};

static camera_metadata_rational COLOR_MATRIX1_2L7_3X3[] =
{
    {593, 1024},  {78, 1024},  {-118, 1024},
    {-491, 1024}, {1412, 1024},{72, 1024},
    {-137, 1024}, {417, 1024}, {428, 1024}
};

static camera_metadata_rational COLOR_MATRIX2_2L7_3X3[] =
{
    {1032, 1024}, {-174, 1024}, {-276, 1024},
    {-442, 1024}, {1495, 1024}, {39, 1024},
    {-76, 1024},  {333, 1024},  {614, 1024}
};

static camera_metadata_rational FORWARD_MATRIX1_2L7_3X3[] =
{
    {893, 1024}, {-200, 1024}, {294, 1024},
    {317, 1024}, {697, 1024},  {10, 1024},
    {-16, 1024}, {-557, 1024}, {1418, 1024}
};

static camera_metadata_rational FORWARD_MATRIX2_2L7_3X3[] =
{
    {750, 1024}, {-150, 1024}, {387, 1024},
    {186, 1024}, {748, 1024},  {90, 1024},
    {-55, 1024}, {-987, 1024}, {1887, 1024}
};
#endif
