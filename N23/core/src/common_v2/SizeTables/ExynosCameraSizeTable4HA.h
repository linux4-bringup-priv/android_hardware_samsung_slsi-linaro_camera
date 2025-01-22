/*
**
** Copyright 2018, Samsung Electronics Co. LTD
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

#ifndef EXYNOS_CAMERA_LUT_4HA_H
#define EXYNOS_CAMERA_LUT_4HA_H

/* -------------------------
    SIZE_RATIO_16_9 = 0,
    SIZE_RATIO_4_3,
    SIZE_RATIO_1_1,
    SIZE_RATIO_3_2,
    SIZE_RATIO_5_4,
    SIZE_RATIO_5_3,
    SIZE_RATIO_11_9,
    SIZE_RATIO_9_16,
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


static int PREVIEW_SIZE_LUT_4HA[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (3264 + 0) ,(1836 + 0),   /* [sensor ] */
      3264      , 1836      ,   /* [bns    ] */
      3264      , 1836      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264      , 2448       ,  /* [bns    ] */
      3264      , 2448      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2448 + 0) ,(2448 + 0),   /* [sensor ] */
      2448      , 2448      ,   /* [bns    ] */
      2448      , 2448      ,   /* [bcrop  ] */
      1080      , 1080      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264      , 2448       ,  /* [bns    ] */
      3264      , 2176      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264      , 2448       ,  /* [bns    ] */
      3060      , 2448      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=3060, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264      , 2448       ,  /* [bns    ] */
      3264      , 1958      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264      , 2448       ,  /* [bns    ] */
      2992      , 2448      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    },
    /* 9:16 (Single, Dual) */
    { SIZE_RATIO_9_16,
     (3264 + 0) ,(1836 + 0),   /* [sensor ] */
      3264      , 1836      ,   /* [bns    ] */
      3264      , 1836      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (3264 + 0) ,(1588 + 0) ,   /* [sensor ] */
      3264      , 1588      ,   /* [bns    ] */
      3264      , 1588      ,   /* [bcrop  ] */
      2224      , 1080      ,   /* [bds    ] */
      2224      , 1080      ,   /* [target ] */
    }
};

static int PICTURE_SIZE_LUT_4HA[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (3264 + 0) ,(1836 + 0),   /* [sensor ] */
      3264      , 1836      ,   /* [bns    ] */
      3264      , 1836      ,   /* [bcrop  ] */
      3264      , 1836      ,   /* [bds    ] */
      3264      , 1836      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264      , 2448      ,   /* [bns    ] */
      3264      , 2448      ,   /* [bcrop  ] */
      3264      , 2448      ,   /* [bds    ] */
      3264      , 2448      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2448 + 0) ,(2448 + 0),   /* [sensor ] */
      2448      , 2448      ,   /* [bns    ] */
      2448      , 2448      ,   /* [bcrop  ] */
      2448      , 2448      ,   /* [bds    ] */
      2448      , 2448      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264	    , 2448      ,  /* [bns    ] */
      3264	    , 2176      ,  /* [bcrop  ] */
      3264      , 2176      ,  /* [bds    ] */
      3264      , 2176      ,  /* [target ] */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
       3264     , 2448      ,  /* [bns    ] */
       3060     , 2448      ,  /* [bcrop  ] */
       3060     , 2448      ,  /* [bds    ] */
       3060     , 2448      ,  /* [target ] */
    },
    /*	5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264      , 2448      ,  /* [bns    ] */
      3264      , 1958      ,  /* [bcrop  ] */
      3264      , 1958      ,  /* [bds    ] */
      3264      , 1958      ,  /* [target ] */
    },
    /*	11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
       3264     , 2448      ,  /* [bns    ] */
       2992     , 2448      ,  /* [bcrop  ] */
       2992     , 2448      ,  /* [bds    ] */
       2992     , 2448      ,  /* [target ] */
    },
    /* 9:16 (Single, Dual) */
    { SIZE_RATIO_9_16,
     (3264 + 0) ,(1836 + 0),   /* [sensor ] */
      3264      , 1836      ,   /* [bns    ] */
      3264      , 1836      ,   /* [bcrop  ] */
      3264      , 1836      ,   /* [bds    ] */
      3264      , 1836      ,   /* [target ] */
    },
    /*  18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (3264 + 0) ,(1588 + 0) ,   /* [sensor ] */
      3264      , 1588      ,   /* [bns    ] */
      3264      , 1588      ,   /* [bcrop  ] */
      3264      , 1588      ,   /* [bds    ] */
      3264      , 1588      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_4HA[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /*  16:9 (Single) */
    { SIZE_RATIO_16_9,
     (3264 + 0) ,(1836 + 0),   /* [sensor ] */
      3264      , 1836      ,   /* [bns    ] */
      3264      , 1836      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single) */
    { SIZE_RATIO_4_3,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264      , 2448      ,   /* [bns    ] */
      3264      , 2448      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2448 + 0) ,(2448 + 0),   /* [sensor ] */
      2448      , 2448      ,   /* [bns    ] */
      2448      , 2448      ,   /* [bcrop  ] */
      1080      , 1080      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264      , 2448      ,   /* [bns    ] */
      3264      , 2176      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264      , 2448      ,   /* [bns    ] */
      3060      , 2448      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=3060, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264      , 2448      ,   /* [bns    ] */
      3264      , 1958      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264      , 2448      ,   /* [bns    ] */
      2992      , 2448      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    },
    /* 9:16 (Single, Dual) */
    { SIZE_RATIO_9_16,
     (3264 + 0) ,(1836 + 0),   /* [sensor ] */
      3264      , 1836      ,   /* [bns    ] */
      3264      , 1836      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (3264 + 0) ,(1588 + 0) ,   /* [sensor ] */
      3264      , 1588      ,   /* [bns    ] */
      3264      , 1588      ,   /* [bcrop  ] */
      2224      , 1080      ,   /* [bds    ] */
      2224      , 1080      ,   /* [target ] */
    }
};

static int VTCALL_SIZE_LUT_4HA[][SIZE_OF_LUT] =
{
    /* 16:9 (VT_Call) */
    { SIZE_RATIO_16_9,
      1632      , 1224      ,   /* [sensor ] */
      1632      , 1224      ,   /* [bns    ] */
      1632      , 918       ,   /* [bcrop  ] */
      1632      , 918       ,   /* [bds    ] */
      1280      , 720       ,   /* [target ] */
    },
    { SIZE_RATIO_4_3,
      1632      , 1224      ,   /* [sensor ] */
      1632      , 1224      ,   /* [bns    ] */
      1632      , 1224      ,   /* [bcrop  ] */
      1632      , 1224      ,   /* [bds    ] */
      960       , 720       ,   /* [target ] */
    },
    { SIZE_RATIO_1_1,
      1632      , 1224      ,   /* [sensor ] */
      1632      , 1224      ,   /* [bns    ] */
      1200      , 1200      ,   /* [bcrop  ] */
      1080      , 1080      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    { SIZE_RATIO_11_9,
      1632      , 1224      ,   /* [sensor ] */
      1632      , 1224      ,   /* [bns    ] */
      1496      , 1218      ,   /* [bcrop  ] */
      1168      , 960       ,   /* [bds    ] */
      1168      , 960       ,   /* [target ] */
    }
};

static int FAST_AE_STABLE_SIZE_LUT_4HA[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */
    /*  4:3 (Single) */
    { SIZE_RATIO_4_3,
     ( 800 + 0) ,( 600 + 0),   /* [sensor ] *//* Sensor binning ratio = 4 */
       800      ,  600      ,   /* [bns    ] */
       800      ,  600      ,   /* [bcrop  ] */
       800      ,  600      ,   /* [bds    ] */
       800      ,  600      ,   /* [target ] */
    }
};

static int PREVIEW_FULL_SIZE_LUT_4HA[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (3264 + 0) ,(1836 + 0),   /* [sensor ] */
      3264      , 1836      ,   /* [bns    ] */
      3264      , 1836      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264      , 2448       ,  /* [bns    ] */
      3264      , 2448      ,   /* [bcrop  ] */
      1440      , 1080      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2448 + 0) ,(2448 + 0),   /* [sensor ] */
      2448      , 2448      ,   /* [bns    ] */
      2448      , 2448      ,   /* [bcrop  ] */
      1080      , 1080      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264      , 2448       ,  /* [bns    ] */
      3264      , 2176      ,   /* [bcrop  ] */
      1616      , 1080      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264      , 2448       ,  /* [bns    ] */
      3060      , 2448      ,   /* [bcrop  ] */
      1344      , 1080      ,   /* [bds    ] *//* w=3060, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264      , 2448       ,  /* [bns    ] */
      3264      , 1958      ,   /* [bcrop  ] */
      1792      , 1080      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264      , 2448       ,  /* [bns    ] */
      2992      , 2448      ,   /* [bcrop  ] */
      1312      , 1080      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    },
    /* 9:16 (Single, Dual) */
    { SIZE_RATIO_9_16,
     (3264 + 0) ,(1836 + 0),   /* [sensor ] */
      3264      , 1836      ,   /* [bns    ] */
      3264      , 1836      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (3264 + 0) ,(1588 + 0) ,   /* [sensor ] */
      3264      , 1588      ,   /* [bns    ] */
      3264      , 1588      ,   /* [bcrop  ] */
      2224      , 1080      ,   /* [bds    ] */
      2224      , 1080      ,   /* [target ] */
    }
};

static int PICTURE_FULL_SIZE_LUT_4HA[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (3264 + 0) ,(1836 + 0),   /* [sensor ] */
      3264      , 1836      ,   /* [bns    ] */
      3264      , 1836      ,   /* [bcrop  ] */
      3264      , 1836      ,   /* [bds    ] */
      3264      , 1836      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264      , 2448      ,   /* [bns    ] */
      3264      , 2448      ,   /* [bcrop  ] */
      3264      , 2448      ,   /* [bds    ] */
      3264      , 2448      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2448 + 0) ,(2448 + 0),   /* [sensor ] */
      2448      , 2448      ,   /* [bns    ] */
      2448      , 2448      ,   /* [bcrop  ] */
      2448      , 2448      ,   /* [bds    ] */
      2448      , 2448      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264	    , 2448      ,  /* [bns    ] */
      3264	    , 2176      ,  /* [bcrop  ] */
      3264      , 2176      ,  /* [bds    ] */
      3264      , 2176      ,  /* [target ] */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
       3264     , 2448      ,  /* [bns    ] */
       3060     , 2448      ,  /* [bcrop  ] */
       3060     , 2448      ,  /* [bds    ] */
       3060     , 2448      ,  /* [target ] */
    },
    /*	5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
      3264      , 2448      ,  /* [bns    ] */
      3264      , 1958      ,  /* [bcrop  ] */
      3264      , 1958      ,  /* [bds    ] */
      3264      , 1958      ,  /* [target ] */
    },
    /*	11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (3264 + 0) ,(2448 + 0),   /* [sensor ] */
       3264     , 2448      ,  /* [bns    ] */
       2992     , 2448      ,  /* [bcrop  ] */
       2992     , 2448      ,  /* [bds    ] */
       2992     , 2448      ,  /* [target ] */
    },
    /* 9:16 (Single, Dual) */
    { SIZE_RATIO_9_16,
     (3264 + 0) ,(1836 + 0),   /* [sensor ] */
      3264      , 1836      ,   /* [bns    ] */
      3264      , 1836      ,   /* [bcrop  ] */
      3264      , 1836      ,   /* [bds    ] */
      3264      , 1836      ,   /* [target ] */
    },
    /*  18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (3264 + 0) ,(1588 + 0) ,   /* [sensor ] */
      3264      , 1588      ,   /* [bns    ] */
      3264      , 1588      ,   /* [bcrop  ] */
      3264      , 1588      ,   /* [bds    ] */
      3264      , 1588      ,   /* [target ] */
    }
};

static int S5K4HA_YUV_LIST[][SIZE_OF_RESOLUTION] =
{
    { 3264, 2448, 33331760, SIZE_RATIO_4_3},
    { 3264, 1836, 33331760, SIZE_RATIO_16_9},
    { 2448, 2448, 33331760, SIZE_RATIO_1_1},
    { 2048, 1536, 33331760, SIZE_RATIO_4_3},
    { 2048, 1152, 33331760, SIZE_RATIO_16_9},
    { 1920, 1920, 33331760, SIZE_RATIO_1_1},
    { 1920, 1080, 33331760, SIZE_RATIO_16_9},
    { 1440, 1080, 33331760, SIZE_RATIO_4_3},
    { 1280,  720, 16665880, SIZE_RATIO_16_9},
    { 1088, 1088, 16665880, SIZE_RATIO_1_1},
    { 1056,  704, 16665880, SIZE_RATIO_3_2},
    {  960,  720, 16665880, SIZE_RATIO_4_3},
    {  800,  480, 16665880, SIZE_RATIO_5_3},
    {  736,  736, 16665880, SIZE_RATIO_1_1},
    {  720,  480, 16665880, SIZE_RATIO_3_2},
    {  640,  480, 16665880, SIZE_RATIO_4_3},
    {  352,  288, 16665880, SIZE_RATIO_11_9},
    {  320,  240, 16665880, SIZE_RATIO_4_3},
    {  176,  144, 16665880, SIZE_RATIO_11_9}
};

static int S5K4HA_JPEG_LIST[][SIZE_OF_RESOLUTION] =
{
    { 3264, 2448, 50000000, SIZE_RATIO_4_3},
    { 3264, 1836, 50000000, SIZE_RATIO_16_9},
    { 2448, 2448, 50000000, SIZE_RATIO_1_1},
    { 2560, 1920, 50000000, SIZE_RATIO_4_3},
    { 2560, 1440, 50000000, SIZE_RATIO_16_9},
    { 2048, 1536, 50000000, SIZE_RATIO_4_3},
    { 2048, 1152, 50000000, SIZE_RATIO_16_9},
    { 1920, 1080, 33331760, SIZE_RATIO_16_9},
    { 1280,  720, 33331760, SIZE_RATIO_16_9},
    {  960,  720, 33331760, SIZE_RATIO_4_3},
    {  640,  480, 33331760, SIZE_RATIO_4_3},
};

static int S5K4HA_HIDDEN_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1440, 1440, 16665880, SIZE_RATIO_1_1}, /* for 1440*1440 recording*/
    { 2224, 1080, 16665880, SIZE_RATIO_18P5_9},
};

static int S5K4HA_HIDDEN_JPEG_LIST[][SIZE_OF_RESOLUTION] =
{
    { 3264, 1588, 50000000, SIZE_RATIO_18P5_9},
};

static int S5K4HA_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1920, 1080, SIZE_RATIO_16_9},
    { 1440, 1080, SIZE_RATIO_4_3},
    { 1072, 1072, SIZE_RATIO_1_1},
    { 1280,  720, SIZE_RATIO_16_9},
    {  960,  720, SIZE_RATIO_4_3},
    {  800,  450, SIZE_RATIO_16_9},
    {  720,  480, SIZE_RATIO_3_2},
    {  640,  480, SIZE_RATIO_4_3},
    {  480,  320, SIZE_RATIO_3_2},
    {  352,  288, SIZE_RATIO_11_9},
    {  320,  240, SIZE_RATIO_4_3},
    {  176,  144, SIZE_RATIO_11_9}
};

static int S5K4HA_HIGH_SPEED_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
    { 800, 600, 0, SIZE_RATIO_4_3},
};

static int S5K4HA_FPS_RANGE_LIST[][2] =
{
    {   7000,   7000},
    {  15000,  15000},
    {  24000,  24000},
    {   8000,  30000},
    {  10000,  30000},
    {  15000,  30000},
    {  30000,  30000},
};

static int S5K4HA_HIGH_SPEED_VIDEO_FPS_RANGE_LIST[][2] =
{
    {  30000,  60000},
    { 120000, 120000},
};

#endif
