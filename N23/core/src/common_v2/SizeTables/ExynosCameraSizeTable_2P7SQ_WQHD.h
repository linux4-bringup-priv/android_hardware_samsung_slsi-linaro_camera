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

#ifndef EXYNOS_CAMERA_LUT_2P7SQ_H
#define EXYNOS_CAMERA_LUT_2P7SQ_H

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

static int PREVIEW_SIZE_LUT_2P7SQ[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1440p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4608 + 0) ,(2624 + 0),   /* [sensor ] */
      4608      , 2624      ,   /* [bns    ] */
      4608      , 2592      ,   /* [bcrop  ] */
      2304      , 1296      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4608 + 0) ,(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      2304      , 1728      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      3456      , 3456      ,   /* [bcrop  ] */
      1728      , 1728      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3072      ,   /* [bcrop  ] */
      2304      , 1536      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4320      , 3456      ,   /* [bcrop  ] */
      2160      , 1728      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 2768      ,   /* [bcrop  ] */
      2304      , 1384      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4224      , 3456      ,   /* [bcrop  ] */
      2112      , 1728      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] */
    },
    /* Dummy (not used) */
    { SIZE_RATIO_9_16,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 2592      ,   /* [bcrop  ] */
      2304      , 1296      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 2240      ,   /* [bcrop  ] */
      2304      , 1120      ,   /* [bds    ] */
      2224      , 1080      ,   /* [target ] */
    }
};

static int PREVIEW_SIZE_LUT_2P7SQ_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.5
       BDS       = 1440p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4032 + 0) ,(2268 + 0),   /* [sensor ] */
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
     (3024 + 0),(3024 + 0),   /* [sensor ] */
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
    },
    /* Dummy (not used) */
    { SIZE_RATIO_9_16,
     (4032 + 0) ,(3024 + 0) ,   /* [sensor ] */
      4032      , 3024      ,   /* [bns    ] */
      3696      , 3024      ,   /* [bcrop  ] */
      2464      , 2016      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (4032 + 0), (1960 + 0) ,   /* [sensor ] */
      2688      , 1306      ,   /* [bns    ] */
      2688      , 1306      ,   /* [bcrop  ] */
      2688      , 1306      ,   /* [bds    ] */
      2224      , 1080      ,   /* [target ] */
    }
};

static int PICTURE_SIZE_LUT_2P7SQ[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4608 + 0),(2624 + 0),   /* [sensor ] */
      4608      , 2624      ,   /* [bns    ] */
      4608      , 2624      ,   /* [bcrop  ] */
      4608      , 2624      ,   /* [bds    ] */
      4608      , 2624      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      4608      , 3456      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (3456 + 0),(3456 + 0),   /* [sensor ] */
      3456      , 3456      ,   /* [bns    ] */
      3456      , 3456      ,   /* [bcrop  ] */
      3456      , 3456      ,   /* [bds    ] */
      3456      , 3456      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      4608      , 3456      ,   /* [target ] */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      4608      , 3456      ,   /* [target ] */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      4608      , 3456      ,   /* [target ] */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      4608      , 3456      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (4608 + 0), (2240 + 0) ,   /* [sensor ] */
      4608      , 2240      ,   /* [bns    ] */
      4608      , 2240      ,   /* [bcrop  ] */
      4608      , 2240      ,   /* [bds    ] */
      4608      , 2240      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_2P7SQ[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1440p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4608 + 0) ,(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4608 + 0) ,(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      3072      , 2304      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      3456      , 3456      ,   /* [bcrop  ] */
      2304      , 2304      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3072      ,   /* [bcrop  ] */
      3072      , 2048      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4320      , 3456      ,   /* [bcrop  ] */
      2880      , 2304      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 2784      ,   /* [bcrop  ] */
      3072      , 1856      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4224      , 3456      ,   /* [bcrop  ] */
      2816      , 2304      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] */
    },
    /* Dummy (not used) */
    { SIZE_RATIO_9_16,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      3696      , 3024      ,   /* [bcrop  ] */
      2464      , 2016      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (4608 + 0) ,(2240 + 0) ,   /* [sensor ] */
      4608      , 2240      ,   /* [bns    ] */
      4608      , 2240      ,   /* [bcrop  ] */
      3072      , 1494      ,   /* [bds    ] */
      2224      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P7SQ[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */

    /* HD_120 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (1152 + 0) , (864 + 0) ,   /* [sensor ] */
      1152      ,  864      ,   /* [bns    ] */
      1152      ,  648      ,   /* [bcrop  ] */
      1152      ,  648      ,   /* [bds    ] */
      1152      ,  648      ,   /* [target ] */
    },
    /* HD_120 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (1152 + 0) , (864 + 0) ,   /* [sensor ] */
      1152      ,  864      ,   /* [bns    ] */
      1152      ,  864      ,   /* [bcrop  ] */
      1152      ,  864      ,   /* [bds    ] */
      1152      ,  864      ,   /* [target ] */
    },
    /* HD_120 1:1 (Single) */
    { SIZE_RATIO_1_1,
     (1152 + 0) , (864 + 0) ,   /* [sensor ] */
      1152      ,  864      ,   /* [bns    ] */
       720      ,  720      ,   /* [bcrop  ] */
       720      ,  720      ,   /* [bds    ] */
       720      ,  720      ,   /* [target ] */
    },
    /* HD_120 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (1152 + 0) , (864 + 0) ,   /* [sensor ] */
      1152      ,  864      ,   /* [bns    ] */
      1056      ,  704      ,   /* [bcrop  ] */
      1056      ,  704      ,   /* [bds    ] */
      1056      ,  704      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_2P7SQ[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */

    /* HD_120 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (2304 + 0) ,(1312 + 0) ,   /* [sensor ] */
      2304      , 1312      ,   /* [bns    ] */
      1280      ,  720      ,   /* [bcrop  ] */
      1280      ,  720      ,   /* [bds    ] */
      1280      ,  720      ,   /* [target ] */
    },
    /* HD_120 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (2304 + 0) ,(1312 + 0) ,   /* [sensor ] */
      2304      , 1312      ,   /* [bns    ] */
       960      ,  720      ,   /* [bcrop  ] */
       960      ,  720      ,   /* [bds    ] */
       960      ,  720      ,   /* [target ] */
    },
    /* HD_120 1:1 (Single) */
    { SIZE_RATIO_1_1,
     (2304 + 0) ,(1312 + 0) ,   /* [sensor ] */
      2304      , 1312      ,   /* [bns    ] */
       720      ,  720      ,   /* [bcrop  ] */
       720      ,  720      ,   /* [bds    ] */
       720      ,  720      ,   /* [target ] */
    },
    /* HD_120 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (2304 + 0) ,(1312 + 0) ,   /* [sensor ] */
      2304      , 1312      ,   /* [bns    ] */
      1680      , 1120      ,   /* [bcrop  ] */
      1056      ,  704      ,   /* [bds    ] */
      1056      ,  704      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_SSM_2P7SQ[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */

    /* HD_240 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (2304 + 0) ,(1312 + 0) ,   /* [sensor ] */
      2304      , 1312      ,   /* [bns    ] */
      1280      ,  720      ,   /* [bcrop  ] */
      1280      ,  720      ,   /* [bds    ] *//* UHD (3840x2160) special handling in ExynosCameraParameters class */
      1280      ,  720      ,   /* [target ] */
    },
};

static int VTCALL_SIZE_LUT_2P7SQ[][SIZE_OF_LUT] =
{
    /* Binning   = 2
       BNS ratio = 1.0
       BDS       = ON */

    /* 16:9 (VT_Call) */
    { SIZE_RATIO_16_9,
     (2304 + 0) ,(1312 + 0) ,   /* [sensor ] */
      2304      , 1312      ,   /* [bns    ] */
      2016      , 1134      ,   /* [bcrop  ] */
      2016      , 1134      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /* 4:3 (VT_Call) */
    { SIZE_RATIO_4_3,
     (2304 + 0) ,(1728 + 0) ,   /* [sensor ] */
      2304      , 1728      ,   /* [bns    ] */
      2016      , 1512      ,   /* [bcrop  ] */
      2016      , 1512      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (VT_Call) */
    { SIZE_RATIO_1_1,
     (2304 + 0) ,(1728 + 0) ,   /* [sensor ] */
      2304      , 1728      ,   /* [bns    ] */
      1504      , 1504      ,   /* [bcrop  ] */
      1504      , 1504      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    /* 3:2 (VT_Call) */
    { SIZE_RATIO_3_2,
     (2304 + 0) ,(1728 + 0) ,   /* [sensor ] */
      2304      , 1728      ,   /* [bns    ] */
      2016      , 1344      ,   /* [bcrop  ] */
      2016      , 1344      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    /* 11:9 (VT_Call) */
    { SIZE_RATIO_11_9,
     (2304 + 0) ,(1728 + 0) ,   /* [sensor ] */
      2304      , 1728      ,   /* [bns    ] */
      1848      , 1512      ,   /* [bcrop  ] */
      1584      , 1296      ,   /* [bds    ] */
      1232      , 1008      ,   /* [target ] */
    }
};

static int FAST_AE_STABLE_SIZE_LUT_2P7SQ[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 4.0 / FPS = 120
       BDS       = ON */

    /* FAST_AE 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (1152 + 0) , (864 + 0) ,   /* [sensor ] */
      1152      ,  864      ,   /* [bns    ] */
      1152      ,  864      ,   /* [bcrop  ] */
      1152      ,  864      ,   /* [bds    ] */
      1152      ,  864      ,   /* [target ] */
    },
};

static int PREVIEW_FULL_SIZE_LUT_2P7SQ[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* Dummy (not used) */
    { SIZE_RATIO_9_16,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      2688      , 2016      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    }
};

static int PICTURE_FULL_SIZE_LUT_2P7SQ[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      4608      , 3456      ,   /* [target ] */
    },
    /* 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      4608      , 3456      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      4608      , 3456      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      4608      , 3456      ,   /* [target ] */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      4608      , 3456      ,   /* [target ] */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      4608      , 3456      ,   /* [target ] */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      4608      , 3456      ,   /* [target ] */
    },
    /* Dummy (not used) */
    { SIZE_RATIO_9_16,
     (4608 + 0) ,(3456 + 0) ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      4608      , 3456      ,   /* [target ] */
    }
};

/* yuv reprocessing input stream size list */
static int SAK2P7SQ_YUV_REPROCESSING_INPUT_LIST[][SIZE_OF_RESOLUTION] =
{
    { 4608, 3456, 33331760, SIZE_RATIO_4_3 },
};

/* Raw output stream size list */
static int SAK2P7SQ_RAW_OUTPUT_LIST[][SIZE_OF_RESOLUTION] =
{
    { 4608, 3456, 33331760, SIZE_RATIO_4_3 },
};

/* yuv stream size list */
static int SAK2P7SQ_YUV_LIST[][SIZE_OF_RESOLUTION] =
{
    /* { width, height, minFrameDuration, ratioId } */
    { 4608, 3456, 33331760, SIZE_RATIO_4_3},
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
static int SAK2P7SQ_JPEG_LIST[][SIZE_OF_RESOLUTION] =
{
    /* { width, height, minFrameDuration, ratioId } */
    { 4608, 3456, 33331760, SIZE_RATIO_4_3},
    { 4032, 3024, 50000000, SIZE_RATIO_4_3},
    { 4032, 2268, 50000000, SIZE_RATIO_16_9},
    { 3024, 3024, 50000000, SIZE_RATIO_1_1},
    { 3984, 2988, 50000000, SIZE_RATIO_4_3},
    { 3840, 2160, 33331760, SIZE_RATIO_16_9},
    { 3264, 2448, 50000000, SIZE_RATIO_4_3},
    { 3264, 1836, 50000000, SIZE_RATIO_16_9},
    { 2976, 2976, 50000000, SIZE_RATIO_1_1},
    { 2880, 2160, 33331760, SIZE_RATIO_4_3},
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


/* vendor static info : hidden preview size list */
static int SAK2P7SQ_HIDDEN_PREVIEW_SIZE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1440, 1440, 16665880, SIZE_RATIO_1_1}, /* for 1440*1440 recording*/
    { 2224, 1080, 16665880, SIZE_RATIO_18P5_9},
};

/* vendor static info : hidden picture size list */
static int SAK2P7SQ_HIDDEN_PICTURE_SIZE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 4032, 1960, 50000000, SIZE_RATIO_18P5_9},
};

/* For HAL3 */
static int SAK2P7SQ_HIGH_SPEED_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
    //{ 1920,  1080, 120, SIZE_RATIO_16_9},
    { 1280,   720, 120, SIZE_RATIO_16_9},
};

static int SAK2P7SQ_FPS_RANGE_LIST[][2] =
{
    {  15000,  15000},
    {  10000,  24000},
    {  24000,  24000},
    {   7000,  30000},
    {  10000,  30000},
    {  15000,  30000},
    {  30000,  30000},
//    {  60000,  60000},
};

/* For HAL3 */
static int SAK2P7SQ_HIGH_SPEED_VIDEO_FPS_RANGE_LIST[][2] =
{
    {  30000, 120000},
    //{  60000, 120000},
    { 120000, 120000},
    //{  30000, 240000},
    //{  60000, 240000},
    //{ 240000, 240000},
};

/* vendor static info : width, height, min_fps, max_fps, vdis width, vdis height, recording limit time(sec) */
static int SAK2P7SQ_AVAILABLE_VIDEO_LIST[][7] =
{
#ifdef USE_UHD_RECORDING
    { 3840, 2160, 60000, 60000, 0, 0, 300},
    { 3840, 2160, 30000, 30000, 4032, 2268, 600},
#endif
#ifdef USE_WQHD_RECORDING
    { 2560, 1440, 30000, 30000, 3072, 1728, 0},
#endif
    { 2224, 1080, 30000, 30000, 2672, 1296, 0},
    { 1920, 1080, 60000, 60000, 2304, 1296, 600},
    { 1920, 1080, 30000, 30000, 2304, 1296, 0},
    { 1440, 1440, 30000, 30000, 0, 0, 0},
    { 1280,  720, 30000, 30000, 1536, 864, 0},
    {  640,  480, 30000, 30000, 0, 0, 0},
    {  320,  240, 30000, 30000, 0, 0, 0},    /* For support the CameraProvider lib of Message app*/
    {  176,  144, 30000, 30000, 0, 0, 0},    /* For support the CameraProvider lib of Message app*/
};

/*  vendor static info :  width, height, min_fps, max_fps, recording limit time(sec) */
static int SAK2P7SQ_AVAILABLE_HIGH_SPEED_VIDEO_LIST[][5] =
{
    //{ 1920, 1080, 240000, 240000, 0},
    //{ 1280,  720, 240000, 240000, 0},
    { 1280,  720, 120000, 120000, 0},
};

/* effect fps range */
static int SAK2P7SQ_EFFECT_FPS_RANGE_LIST[][2] =
{
    {  10000,  24000},
    {  24000,  24000},
};

static camera_metadata_rational COLOR_MATRIX1_2P7SQ_3X3[] =
{
    {661, 1024}, {-62, 1024}, {-110, 1024},
    {-564, 1024}, {1477, 1024}, {77, 1024},
    {-184, 1024}, {445, 1024}, {495, 1024}
};

static camera_metadata_rational COLOR_MATRIX2_2P7SQ_3X3[] =
{
    {1207, 1024}, {-455, 1024}, {-172, 1024},
    {-488, 1024}, {1522, 1024}, {107, 1024},
    {-82, 1024}, {314, 1024}, {713, 1024}
};

static camera_metadata_rational FORWARD_MATRIX1_2P7SQ_3X3[] =
{
    {759, 1024}, {5, 1024}, {223, 1024},
    {292, 1024}, {732, 1024}, {0, 1024},
    {13, 1024}, {-494, 1024}, {1325, 1024}
};

static camera_metadata_rational FORWARD_MATRIX2_2P7SQ_3X3[] =
{
    {655, 1024}, {68, 1024}, {265, 1024},
    {186, 1024}, {810, 1024}, {28, 1024},
    {-34, 1024}, {-821, 1024}, {1700, 1024}
};


static int SAK2P7SQ_AF_FOV_SIZE_LIST[][6] =
{
    /*
     * lens_shift_max = (Dist_macro_end * focal_length) / (Dist_macro_end - focal_length)
       mag_factor_max = (lens_shift_max / focal_length) * fudge_factor;
     */

    { 0, 1023,  80000000, 2000000, 4544254, 2113606}, /* [MinLensPosition, MaxLensPosition, Dist_macro_end, fudge_factor, lens_shift_max, mag_factor_max] */
};



#endif
