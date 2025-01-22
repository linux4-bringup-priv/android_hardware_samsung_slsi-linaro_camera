/*
**
** Copyright 2016, Samsung Electronics Co. LTD
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

#ifndef EXYNOS_CAMERA_LUT_3J1_H
#define EXYNOS_CAMERA_LUT_3J1_H

/* -------------------------
    SIZE_RATIO_16_9 = 0,
    SIZE_RATIO_4_3,
    SIZE_RATIO_1_1,
    SIZE_RATIO_3_2,
    SIZE_RATIO_5_4,
    SIZE_RATIO_5_3,
    SIZE_RATIO_11_9,
    SIZE_RATIO_9_16,
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
    Sensor Margin Width  = 16,
    Sensor Margin Height = 10
-----------------------------*/

static int PREVIEW_SIZE_LUT_3J1[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
      3968      , 2232      ,   /* [sensor ] */
      3968      , 2232      ,   /* [bns    ] */
      3968      , 2232      ,   /* [bcrop  ] */
      2644      , 1488      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3648      , 2736      ,   /* [bcrop  ] */
      2432      , 1824      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
      2736      , 2736      ,   /* [sensor ] */
      2736      , 2736      ,   /* [bns    ] */
      2736      , 2736      ,   /* [bcrop  ] */
      1824      , 1824      ,   /* [bds    ] *//* w=1080, Increased for 16 pixel align */
      1080      , 1080      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3264      , 2176      ,   /* [bcrop  ] */
      2448      , 1632      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] *//* w=1620, Reduced for 16 pixel align */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3040      , 2432      ,   /* [bcrop  ] */
      2240      , 1792      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] *//* w=1350, Reduced for 16 pixel align */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3240      , 1944      ,   /* [bcrop  ] */
      2560      , 1536      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] *//* w=1800, Reduced for 16 pixel align */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      2992      , 2448      ,   /* [bcrop  ] */
      2112      , 1728      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] *//* w=1320, Reduced for 16 pixel align */
    },
    /*	18.5:9 (Single) */
    { SIZE_RATIO_18P5_9,
      3968      , 1928      ,	/* [sensor ] */
      3968      , 1928      ,	/* [bns    ] */
      3968      , 1928      ,	/* [bcrop  ] */
      2644      , 1284      ,	/* [bds    ] *//* w=2224, Increased for 16 pixel align */
      2224      , 1080      ,	/* [target ] */
    }
};

static int DUAL_PREVIEW_SIZE_LUT_3J1[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
      3968      , 2232      ,   /* [sensor ] */
      3968      , 2232      ,   /* [bns    ] */
      3968      , 2232      ,   /* [bcrop  ] */
      2644      , 1488      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3648      , 2736      ,   /* [bcrop  ] */
      2432      , 1824      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
      2736      , 2736      ,   /* [sensor ] */
      2736      , 2736      ,   /* [bns    ] */
      2736      , 2736      ,   /* [bcrop  ] */
      1824      , 1824      ,   /* [bds    ] *//* w=1080, Increased for 16 pixel align */
      1080      , 1080      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3264      , 2176      ,   /* [bcrop  ] */
      2448      , 1632      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] *//* w=1620, Reduced for 16 pixel align */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3040      , 2432      ,   /* [bcrop  ] */
      2240      , 1792      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] *//* w=1350, Reduced for 16 pixel align */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3240      , 1944      ,   /* [bcrop  ] */
      2560      , 1536      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] *//* w=1800, Reduced for 16 pixel align */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      2992      , 2448      ,   /* [bcrop  ] */
      2112      , 1728      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] *//* w=1320, Reduced for 16 pixel align */
    },
    /*	18.5:9 (Single) */
    { SIZE_RATIO_18P5_9,
      3968      , 1928      ,	/* [sensor ] */
      3968      , 1928      ,	/* [bns    ] */
      3968      , 1928      ,	/* [bcrop  ] */
      2644      , 1284      ,	/* [bds    ] *//* w=2224, Increased for 16 pixel align */
      2224      , 1080      ,	/* [target ] */
    }
};

static int PICTURE_SIZE_LUT_3J1[][SIZE_OF_LUT] =
{
    { SIZE_RATIO_16_9,
      3968      , 2232      ,   /* [sensor ] */
      3968      , 2232      ,   /* [bns    ] */
      3968      , 2232      ,   /* [bcrop  ] */
      3968      , 2232      ,   /* [bds    ] */
      3648      , 2052      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3648      , 2736      ,   /* [bcrop  ] */
      3648      , 2736      ,   /* [bds    ] */
      3648      , 2736      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
      2736      , 2736      ,   /* [sensor ] */
      2736      , 2736      ,   /* [bns    ] */
      2736      , 2736      ,   /* [bcrop  ] */
      2736      , 2736      ,   /* [bds    ] *//* w=1080, Increased for 16 pixel align */
      2736      , 2736      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3264      , 2176      ,   /* [bcrop  ] */
      3264      , 2176      ,   /* [bds    ] */
      3264      , 2176      ,   /* [target ] *//* w=1620, Reduced for 16 pixel align */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3040      , 2432      ,   /* [bcrop  ] */
      3040      , 2432      ,   /* [bds    ] */
      3040      , 2432      ,   /* [target ] *//* w=1350, Reduced for 16 pixel align */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3240      , 1944      ,   /* [bcrop  ] */
      3240      , 1944      ,   /* [bds    ] */
      3240      , 1944      ,   /* [target ] *//* w=1800, Reduced for 16 pixel align */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      2992      , 2448      ,   /* [bcrop  ] */
      2992      , 2448      ,   /* [bds    ] */
      2992      , 2448      ,   /* [target ] *//* w=1320, Reduced for 16 pixel align */
    },
    /* 18.5:9 (Single) */
    { SIZE_RATIO_18P5_9,
      3968      , 1928      ,	/* [sensor ] */
      3968      , 1928      ,	/* [bns    ] */
      3968      , 1928      ,	/* [bcrop  ] */
      3968      , 1928      ,	/* [bds    ] */
      3648      , 1776      ,	/* [target ] */
    }
};

static int VIDEO_SIZE_LUT_3J1[][SIZE_OF_LUT] =
{
    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
      3968      , 2232      ,   /* [sensor ] */
      3968      , 2232      ,   /* [bns    ] */
      3968      , 2232      ,   /* [bcrop  ] */
      3968      , 2232      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3648      , 2736      ,   /* [bcrop  ] */
      3648      , 2736      ,   /* [bds    ] */
      640       , 480       ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
      2736      , 2736      ,   /* [sensor ] */
      2736      , 2736      ,   /* [bns    ] */
      2736      , 2736      ,   /* [bcrop  ] */
      2736      , 2736      ,   /* [bds    ] *//* w=1080, Increased for 16 pixel align */
      1080      , 1080      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3264      , 2176      ,   /* [bcrop  ] */
      3264      , 2176      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] *//* w=1620, Reduced for 16 pixel align */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3040      , 2432      ,   /* [bcrop  ] */
      3040      , 2432      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] *//* w=1350, Reduced for 16 pixel align */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3240      , 1944      ,   /* [bcrop  ] */
      3240      , 1944      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] *//* w=1800, Reduced for 16 pixel align */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      2992      , 2448      ,   /* [bcrop  ] */
      2992      , 2448      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] *//* w=1320, Reduced for 16 pixel align */
    },
    /*	18.5:9 (Single) */
    { SIZE_RATIO_18P5_9,
      3968      , 1928      ,	/* [sensor ] */
      3968      , 1928      ,	/* [bns    ] */
      3968      , 1928      ,	/* [bcrop  ] */
      3968      , 1928      ,	/* [bds    ] *//* w=2224, Increased for 16 pixel align */
      2224      , 1080      ,	/* [target ] */
    }
};

static int VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_3J1[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */

    /*   HD_60  16:9 (Single) */
    { SIZE_RATIO_16_9,
      1988      ,  1120     ,   /* [sensor ] *//* Sensor binning ratio = 2 */
      1988      ,  1120     ,   /* [bns    ] */
      1988      ,  1120     ,   /* [bcrop  ] */
      1988      ,  1120     ,   /* [bds    ] */
      1280      ,  720      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3J1[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */

    /*  HD_120  4:3 (Single) */
    { SIZE_RATIO_4_3,
       912      ,  684      ,   /* [sensor ] *//* Sensor binning ratio = 4 */
       912      ,  684      ,   /* [bns    ] */
       912      ,  684      ,   /* [bcrop  ] */
       912      ,  684      ,   /* [bds    ] */
       912      ,  684      ,   /* [target ] */
    },
};

static int PREVIEW_FULL_SIZE_LUT_3J1[][SIZE_OF_LUT] =
{
    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
      3968      , 2232      ,   /* [sensor ] */
      3968      , 2232      ,   /* [bns    ] */
      3968      , 2232      ,   /* [bcrop  ] */
      2644      , 1488      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3648      , 2736      ,   /* [bcrop  ] */
      2432      , 1824      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single) */
    { SIZE_RATIO_1_1,
      2736      , 2736      ,   /* [sensor ] */
      2736      , 2736      ,   /* [bns    ] */
      2736      , 2736      ,   /* [bcrop  ] */
      1824      , 1824      ,   /* [bds    ] *//* w=1080, Increased for 16 pixel align */
      1080      , 1080      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3264      , 2176      ,   /* [bcrop  ] */
      2448      , 1632      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] *//* w=1620, Reduced for 16 pixel align */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3040      , 2432      ,   /* [bcrop  ] */
      2240      , 1792      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] *//* w=1350, Reduced for 16 pixel align */
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3240      , 1944      ,   /* [bcrop  ] */
      2560      , 1536      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] *//* w=1800, Reduced for 16 pixel align */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      2992      , 2448      ,   /* [bcrop  ] */
      2112      , 1728      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] *//* w=1320, Reduced for 16 pixel align */
    },
};

static int PICTURE_FULL_SIZE_LUT_3J1[][SIZE_OF_LUT] =
{
    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
      3968      , 2232      ,   /* [sensor ] */
      3968      , 2232      ,   /* [bns    ] */
      3968      , 2232      ,   /* [bcrop  ] */
      3968      , 2232      ,   /* [bds    ] */
      3648      , 2052      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3648      , 2736      ,   /* [bcrop  ] */
      3648      , 2736      ,   /* [bds    ] */
      3648      , 2736      ,   /* [target ] */
    },
    /*  1:1 (Single) */
    { SIZE_RATIO_1_1,
      2736      , 2736      ,   /* [sensor ] */
      2736      , 2736      ,   /* [bns    ] */
      2736      , 2736      ,   /* [bcrop  ] */
      2736      , 2736      ,   /* [bds    ] *//* w=1080, Increased for 16 pixel align */
      2736      , 2736      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3264      , 2176      ,   /* [bcrop  ] */
      3264      , 2176      ,   /* [bds    ] */
      3264      , 2176      ,   /* [target ] *//* w=1620, Reduced for 16 pixel align */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3040      , 2432      ,   /* [bcrop  ] */
      3040      , 2432      ,   /* [bds    ] */
      3040      , 2432      ,   /* [target ] *//* w=1350, Reduced for 16 pixel align */
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3240      , 1944      ,   /* [bcrop  ] */
      3240      , 1944      ,   /* [bds    ] */
      3240      , 1944      ,   /* [target ] *//* w=1800, Reduced for 16 pixel align */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      2992      , 2448      ,   /* [bcrop  ] */
      2992      , 2448      ,   /* [bds    ] */
      2992      , 2448      ,   /* [target ] *//* w=1320, Reduced for 16 pixel align */
    },
};

static int VTCALL_SIZE_LUT_3J1[][SIZE_OF_LUT] = {
    /* Binning   = 2
       BNS ratio = 1.0
       BDS       = ON */

    /* 16:9 (VT_Call) */
    { SIZE_RATIO_16_9,
      1988      , 1120     ,   /* [sensor ] */
      1988      , 1120     ,   /* [bns    ] */
      1988      , 1120     ,   /* [bcrop  ] */
      1324      ,  746     ,   /* [bds    ] */
      1280      ,  720     ,   /* [target ] */
    },
    /* 4:3 (VT_Call) */
    /* Bcrop size 1152*864 -> 1280*960, for flicker algorithm */
    { SIZE_RATIO_4_3,
      1824      ,  1368    ,   /* [sensor ] */
      1824      ,  1368    ,   /* [bns    ] */
      1824      ,  1368    ,   /* [bcrop  ] */
      1216      ,   912    ,   /* [bds    ] */
      640       ,   480    ,   /* [target ] */
    },
    /* 1:1 (VT_Call) */
    { SIZE_RATIO_1_1,
      1824      ,  1824     ,   /* [sensor ] */
      1824      ,  1824     ,   /* [bns    ] */
      1824      ,  1824     ,   /* [bcrop  ] */
      1824      ,  1824     ,   /* [bds    ] */
      1080      ,  1080     ,   /* [target ] */
    },
    /* 11:9 (VT_Call) */
    /* Bcrop size 1056*864 -> 1168*960, for flicker algorithm */
    { SIZE_RATIO_11_9,
      1824      ,  1368     ,   /* [sensor ] */
      1824      ,  1368     ,   /* [bns    ] */
      1496      ,  1224     ,   /* [bcrop  ] */
      1496      ,  1224     ,   /* [bds    ] */
      1168      ,   960     ,   /* [target ] */
    }
};

static int DUAL_VIDEO_SIZE_LUT_3J1[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */
    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
      3976      , 2232      ,   /* [sensor ] */
      3976      , 2232      ,   /* [bns    ] */
      3976      , 2232      ,   /* [bcrop  ] */
      3976      , 2232      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
      3648      , 2736      ,   /* [sensor ] */
      3648      , 2736      ,   /* [bns    ] */
      3648      , 2736      ,   /* [bcrop  ] */
      3648      , 2736      ,   /* [bds    ] */
      640       , 480       ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
      2736      , 2736      ,   /* [sensor ] */
      2736      , 2736      ,   /* [bns    ] */
      2736      , 2736      ,   /* [bcrop  ] */
      2736      , 2736      ,   /* [bds    ] *//* w=1080, Increased for 16 pixel align */
      1080      , 1080      ,   /* [target ] */
    },
};

static int S5K3J1_YUV_LIST[][SIZE_OF_RESOLUTION] =
{
    /* { width, height, minFrameDuration, ratioId } */
    { 3648, 2232, 16665880, SIZE_RATIO_16_9},
    { 3648, 2736, 16665880, SIZE_RATIO_4_3},
    { 2736, 2736, 16665880, SIZE_RATIO_1_1},
    { 2560, 1440, 16665880, SIZE_RATIO_16_9},
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
static int S5K3J1_JPEG_LIST[][SIZE_OF_RESOLUTION] =
{
    /* { width, height, minFrameDuration, ratioId } */
    { 3648, 2052, 50000000, SIZE_RATIO_16_9},
    { 3648, 2736, 50000000, SIZE_RATIO_4_3},
    { 2736, 2736, 50000000, SIZE_RATIO_1_1},
    { 2560, 1440, 50000000, SIZE_RATIO_16_9},
    { 2448, 2448, 50000000, SIZE_RATIO_1_1},
    { 2048, 1536, 50000000, SIZE_RATIO_4_3},
    { 1936, 1936, 50000000, SIZE_RATIO_1_1},
    { 1920, 1440, 50000000, SIZE_RATIO_4_3},
    { 1920, 1080, 33331760, SIZE_RATIO_16_9},
    { 1440, 1080, 33331760, SIZE_RATIO_4_3},
    { 1088, 1088, 33331760, SIZE_RATIO_1_1},
    { 1072, 1072, 33331760, SIZE_RATIO_1_1},
    { 1440, 1440, 33331760, SIZE_RATIO_1_1},
    { 1280,  720, 33331760, SIZE_RATIO_16_9},
    { 1056,  704, 33331760, SIZE_RATIO_3_2},
    {  960,  720, 33331760, SIZE_RATIO_4_3},
    {  800,  600, 33331760, SIZE_RATIO_4_3},
    {  800,  450, 33331760, SIZE_RATIO_16_9},
    {  736,  736, 33331760, SIZE_RATIO_1_1},
    {  720,  480, 33331760, SIZE_RATIO_3_2},
    {  640,  480, 33331760, SIZE_RATIO_4_3},
    {  352,  288, 33331760, SIZE_RATIO_11_9},
    {  320,  240, 33331760, SIZE_RATIO_4_3},
};

/* vendor static info : hidden preview size list */
static int S5K3J1_HIDDEN_PREVIEW_SIZE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 2224, 1080, 33331760, SIZE_RATIO_18P5_9},
    { 1440, 1440, 16665880, SIZE_RATIO_1_1}, /* for 1440*1440 recording*/
};

/* vendor static info : hidden picture size list */
static int S5K3J1_HIDDEN_PICTURE_SIZE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 3648, 1776, 50000000, SIZE_RATIO_18P5_9},
};

/* vendor static info : width, height, min_fps, max_fps, vdis width, vdis height, recording limit time(sec) */
static int S5K3J1_AVAILABLE_VIDEO_LIST[][7] =
{
#ifdef USE_WQHD_RECORDING
    { 2560, 1440, 30000, 30000, 3072, 1728, 0},
#endif
    { 2224, 1080, 30000, 30000, 2672, 1296, 0},
    { 1440, 1440, 30000, 30000, 0, 0, 0},
    { 1920, 1080, 30000, 30000, 2304, 1296, 0},
    { 1280,  720, 30000, 30000, 1536, 864, 0},
    {  640,  480, 30000, 30000, 0, 0, 0},
    {  320,  240, 30000, 30000, 0, 0, 0},    /* For support the CameraProvider lib of Message app*/
    {  176,  144, 30000, 30000, 0, 0, 0},    /* For support the CameraProvider lib of Message app*/
};

static int S5K3J1_HIGH_SPEED_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
    {  800,  600, 0, SIZE_RATIO_4_3},
};

static int S5K3J1_FPS_RANGE_LIST[][2] =
{
    {  10000,  24000},
    {  24000,  24000},
    {   7000,  30000},
    {   8000,  30000},
    {  10000,  30000},
    {  15000,  30000},
    {  30000,  30000},
};

static int S5K3J1_HIGH_SPEED_VIDEO_FPS_RANGE_LIST[][2] =
{
    {  30000, 120000},
    {  60000, 120000},
    { 120000, 120000},
};

static camera_metadata_rational COLOR_MATRIX1_3J1[] =
{
    {624, 1024}, {-65, 1024}, {-109, 1024},
    {-559, 1024}, {1457, 1024}, {91, 1024},
    {-153, 1024}, {393, 1024}, {520, 1024}
};

static camera_metadata_rational COLOR_MATRIX2_3J1[] =
{
    {1327, 1024}, {-530, 1024}, {-291, 1024},
    {-611, 1024}, {1691, 1024}, {12, 1024},
    {-98, 1024}, {305, 1024}, {727, 1024}
};

static camera_metadata_rational FORWARD_MATRIX1_3J1[] =
{
    {748, 1024}, {23, 1024}, {217, 1024},
    {290, 1024}, {749, 1024}, {-15, 1024},
    {0, 1024}, {-418, 1024}, {1263, 1024}
};

static camera_metadata_rational FORWARD_MATRIX2_3J1[] =
{
    {622, 1024}, {63, 1024}, {302, 1024},
    {208, 1024}, {720, 1024}, {95, 1024},
    {-15, 1024}, {-683, 1024}, {1543, 1024}
};

#ifdef SUPPORT_PD_IMAGE
static int PD_IMAGE_SIZE_LUT_3J1[][PD_IMAGE_LUT_SIZE] =
{
    /* { SENSOR_W, SENSOR_H, PD_IMAGE_W, PD_IMAGE_H } */
    { 3648,   2736,   /* [sensor ] */
      3648,   684,    /* [Y    ] */
    },

    { 2736,   2736,   /* [sensor ] */
      2736,   684,    /* [Y    ] */
    },

    { 3968,   2232,   /* [sensor ] */
      3968,   558,    /* [Y    ] */
    },

    { 3968,   1880,   /* [sensor ] */
      3968,   470,    /* [Y    ] */
    },

    { 2944,   2208,   /* [sensor ] */
      2944,   552,    /* [Y    ] */
    },

    { 3216,   1808,   /* [sensor ] */
      3216,   452,    /* [Y    ] */
    },

    { 3216,   1528,   /* [sensor ] */
      3216,   382,    /* [Y    ] */
    },

    { 2208,   2208,   /* [sensor ] */
      2208,   552,    /* [Y    ] */
    },

    { 1824,   1368,   /* [sensor ] */
      1824,   342,    /* [Y    ] */
    },

    { 1988,   1120,   /* [sensor ] */
      1984,   280,    /* [Y    ] */
    },

    { 1472,   1104,   /* [sensor ] */
      1472,   276,    /* [Y    ] */
    },

    { 1616,   904,   /* [sensor ] */
      1616,   226,    /* [Y    ] */
    },

    { 1104,   1104,   /* [sensor ] */
      1104,   226,    /* [Y    ] */
    },
};
#endif
#endif
