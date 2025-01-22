/*
**
**copyright 2016, Samsung Electronics Co. LTD
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

#ifndef EXYNOS_CAMERA_LUT_2P6_H
#define EXYNOS_CAMERA_LUT_2P6_H

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
    Sensor Margin Width  = 16,
    Sensor Margin Height = 12
-----------------------------*/

static int PREVIEW_SIZE_LUT_2P6[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4608 + 0),(2592 + 0)  ,   /* [sensor ] */
      4608      , 2592      ,   /* [bns    ] */
      4608      , 2592      ,   /* [bcrop  ] */
      2304      , 1296      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4608 + 0),(3456 + 0)  ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      2304      , 1728      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4608 + 0),(3456 + 0)  ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      3456      , 3456      ,   /* [bcrop  ] */
      1728      , 1728      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4608 + 0),(3456 + 0)  ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3072      ,   /* [bcrop  ] */
      2304      , 1536      ,   /* [bds    ] *//* w=2304, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4608 + 0),(3456 + 0)  ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4320      , 3456      ,   /* [bcrop  ] */
      2160      , 1728      ,   /* [bds    ] *//* w=2160, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4608 + 0),(3456 + 0)  ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4600      , 2760      ,   /* [bcrop  ] */
      2300      , 1380      ,   /* [bds    ] *//* w=2304, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4608 + 0),(3456 + 0)  ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4224      , 3456      ,   /* [bcrop  ] */
      2112      , 1728      ,   /* [bds    ] *//* w=2112, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int PICTURE_SIZE_LUT_2P6[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4608 + 0),(2592 + 0),   /* [sensor ] */
      4608      , 2592      ,   /* [bns    ] */
      4608      , 2592      ,   /* [bcrop  ] */
      4608      , 2592      ,   /* [bds    ] */
      4608      , 2592      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      4608      , 3456      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      3456      , 3456      ,   /* [bcrop  ] */
      3456      , 3456      ,   /* [bds    ] */
      3456      , 3456      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3072      ,   /* [bcrop  ] */
      4608      , 3072      ,   /* [bds    ] */
      4608      , 3072      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4320      , 3456      ,   /* [bcrop  ] */
      4320      , 3456      ,   /* [bds    ] *//* w=3060, Reduced for 16 pixel align */
      4320      , 3456      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4600      , 2760      ,   /* [bcrop  ] */
      4600      , 2760      ,   /* [bds    ] */
      4600      , 2760      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4224      , 3456      ,   /* [bcrop  ] */
      4224      , 3456      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      4224      , 3456      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_2P6[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4608 + 0),(2592 + 0),   /* [sensor ] */
      4608      , 2592      ,   /* [bns    ] */
      4608      , 2592      ,   /* [bcrop  ] */
      2304      , 1296      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      2304      , 1728      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      3456      , 3456      ,   /* [bcrop  ] */
      1728      , 1728      ,   /* [bds    ] */
      1088      , 1088      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3072      ,   /* [bcrop  ] */
      2304      , 1536      ,   /* [bds    ] *//* w=2304, Reduced for 16 pixel align */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4320      , 3456      ,   /* [bcrop  ] */
      2160      , 1728      ,   /* [bds    ] *//* w=2160, Reduced for 16 pixel align */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4600      , 2760      ,   /* [bcrop  ] */
      2300      , 1380      ,   /* [bds    ] *//* w=2304, Reduced for 16 pixel align */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4224      , 3456      ,   /* [bcrop  ] */
      2112      , 1728      ,   /* [bds    ] *//* w=2112, Reduced for 16 pixel align */
      1312      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P6[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 4.0
       BDS       = OFF */

    /*  HD_120  16:9 (Single) */
    { SIZE_RATIO_16_9,
     (1152 + 0),(648 + 0),   /* [sensor ] *//* Sensor binning ratio = 4 */
      1152      , 648      ,   /* [bns    ] */
      1152      , 648      ,   /* [bcrop  ] */
      1152      , 648      ,   /* [bds    ] */
      1152      , 648      ,   /* [target ] */
    },
    /*  HD_120  4:3 (Single) */
    { SIZE_RATIO_4_3,
     (1152 + 0),(864 + 0),   /* [sensor ] *//* Sensor binning ratio = 4 */
      1152      , 864      ,   /* [bns    ] */
      1152      , 864      ,   /* [bcrop  ] */
      1152      , 864      ,   /* [bds    ] */
      1152      , 864      ,   /* [target ] */
    }
};

static int VTCALL_SIZE_LUT_2P6[][SIZE_OF_LUT] =
{
    /* 16:9 (VT_Call) */
    { SIZE_RATIO_16_9,
      2304      , 1296      ,   /* [sensor ] */
      2304      , 1296      ,   /* [bns    ] */
      2304      , 1296      ,   /* [bcrop  ] */
      2304      , 1296      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /* 4:3 (VT_Call) */
    { SIZE_RATIO_4_3,
      2304      , 1728      ,   /* [sensor ] */
      2304      , 1728      ,   /* [bns    ] */
      2304      , 1728      ,   /* [bcrop  ] */
      2304      , 1728      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (VT_Call) */
    { SIZE_RATIO_1_1,
      2304      , 1728      ,   /* [sensor ] */
      2304      , 1728      ,   /* [bns    ] */
      1728      , 1728      ,   /* [bcrop  ] */
      1728      , 1728      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    /* 11:9 (VT_Call) */
    { SIZE_RATIO_11_9,
      2304      , 1728      ,   /* [sensor ] */
      2304      , 1728      ,   /* [bns    ] */
      2112      , 1728      ,   /* [bcrop  ] */
      2112      , 1728      ,   /* [bds    ] */
      1232      , 1008      ,   /* [target ] */
    }
};

static int FAST_AE_STABLE_SIZE_LUT_2P6[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 4.0
       BDS       = OFF */

    /*  FAST_AE 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (1152 + 0),(846 + 0),   /* [sensor ] *//* Sensor binning ratio = 4 */
      1152      , 846      ,   /* [bns    ] */
      1152      , 846      ,   /* [bcrop  ] */
      1152      , 846      ,   /* [bds    ] */
      1152      , 846      ,   /* [target ] */
    }
};

static int PREVIEW_FULL_SIZE_LUT_2P6[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4608 + 0),(3456 + 0)  ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      2304      , 1728      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4608 + 0),(3456 + 0)  ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      2304      , 1728      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4608 + 0),(3456 + 0)  ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      2304      , 1728      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4608 + 0),(3456 + 0)  ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      2304      , 1728      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4608 + 0),(3456 + 0)  ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      2304      , 1728      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4608 + 0),(3456 + 0)  ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      2304      , 1728      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4608 + 0),(3456 + 0)  ,   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      2304      , 1728      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    }
};

static int PICTURE_FULL_SIZE_LUT_2P6[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      4608      , 3456      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      4608      , 3456      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      4608      , 3456      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      4608      , 3456      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] *//* w=3060, Reduced for 16 pixel align */
      4608      , 3456      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] */
      4608      , 3456      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (4608 + 0),(3456 + 0),   /* [sensor ] */
      4608      , 3456      ,   /* [bns    ] */
      4608      , 3456      ,   /* [bcrop  ] */
      4608      , 3456      ,   /* [bds    ] *//* w=1320, Reduced for 16 pixel align */
      4608      , 3456      ,   /* [target ] */
    }
};

static int S5K2P6_YUV_LIST[][SIZE_OF_RESOLUTION] =
{
    /* { width, height, minFrameDuration, ratioId } */
    { 4608, 3456, 33331760, SIZE_RATIO_4_3},
    { 4096, 3072, 33331760, SIZE_RATIO_4_3},
    { 4608, 2592, 33331760, SIZE_RATIO_16_9},
    { 3840, 2160, 33331760, SIZE_RATIO_16_9},
    { 3456, 3456, 33331760, SIZE_RATIO_1_1},
    { 3456, 2592, 33331760, SIZE_RATIO_4_3},
    { 3264, 2448, 33331760, SIZE_RATIO_4_3},
    { 3264, 1836, 33331760, SIZE_RATIO_16_9},
    { 2448, 2448, 33331760, SIZE_RATIO_1_1},
    { 2048, 1152, 33331760, SIZE_RATIO_16_9},
    { 1920, 1440, 33331760, SIZE_RATIO_4_3},
    { 1920, 1080, 33331760, SIZE_RATIO_16_9},
    { 1440, 1080, 33331760, SIZE_RATIO_4_3},
    { 1280,  720, 33331760, SIZE_RATIO_16_9},
    { 1088, 1088, 33331760, SIZE_RATIO_1_1},
    { 1072, 1072, 33331760, SIZE_RATIO_1_1},
    { 1056,  704, 33331760, SIZE_RATIO_3_2},
    { 1024,  768, 33331760, SIZE_RATIO_4_3},
    {  960,  720, 33331760, SIZE_RATIO_4_3},
    {  800,  450, 33331760, SIZE_RATIO_16_9},
    {  720,  720, 33331760, SIZE_RATIO_1_1},
    {  720,  480, 33331760, SIZE_RATIO_3_2},
    {  640,  480, 33331760, SIZE_RATIO_4_3},
    {  480,  320, 33331760, SIZE_RATIO_3_2},
    {  352,  288, 33331760, SIZE_RATIO_11_9},
    {  320,  240, 33331760, SIZE_RATIO_4_3},
    {  256,  144, 33331760, SIZE_RATIO_16_9}, /* DngCreatorTest */
    {  176,  144, 33331760, SIZE_RATIO_11_9}, /* RecordingTest */
};

/* availble Jpeg size (only for  HAL_PIXEL_FORMAT_BLOB) */
static int S5K2P6_JPEG_LIST[][SIZE_OF_RESOLUTION] =
{
    /* { width, height, minFrameDuration, ratioId } */
    { 4608, 3456, 50000000, SIZE_RATIO_4_3},
    { 4096, 3072, 50000000, SIZE_RATIO_4_3},
    { 4608, 2592, 50000000, SIZE_RATIO_16_9},
    { 3840, 2160, 50000000, SIZE_RATIO_16_9},
    { 3456, 3456, 50000000, SIZE_RATIO_1_1},
    { 3456, 2592, 50000000, SIZE_RATIO_4_3},
    { 3264, 2448, 50000000, SIZE_RATIO_4_3},
    { 3264, 1836, 50000000, SIZE_RATIO_16_9},
    { 2448, 2448, 50000000, SIZE_RATIO_1_1},
    { 2048, 1152, 50000000, SIZE_RATIO_16_9},
    { 1920, 1440, 50000000, SIZE_RATIO_4_3},
    { 1920, 1080, 33331760, SIZE_RATIO_16_9},
    { 1440, 1080, 33331760, SIZE_RATIO_4_3},
    { 1280,  720, 33331760, SIZE_RATIO_16_9},
    { 1088, 1088, 33331760, SIZE_RATIO_1_1},
    { 1072, 1072, 33331760, SIZE_RATIO_1_1},
    { 1056,  704, 33331760, SIZE_RATIO_3_2},
    { 1024,  768, 33331760, SIZE_RATIO_4_3},
    {  960,  720, 33331760, SIZE_RATIO_4_3},
    {  800,  450, 33331760, SIZE_RATIO_16_9},
    {  720,  720, 33331760, SIZE_RATIO_1_1},
    {  720,  480, 33331760, SIZE_RATIO_3_2},
    {  640,  480, 33331760, SIZE_RATIO_4_3},
    {  480,  320, 33331760, SIZE_RATIO_3_2},
    {  352,  288, 33331760, SIZE_RATIO_11_9},
    {  320,  240, 33331760, SIZE_RATIO_4_3},
};

static int S5K2P6_THUMBNAIL_LIST[][SIZE_OF_RESOLUTION] =
{
    {  512,  384, SIZE_RATIO_4_3},
    {  512,  288, SIZE_RATIO_16_9},
    {  384,  384, SIZE_RATIO_1_1},
/* TODO : will be supported after enable S/W scaler correctly */
//  {  320,  240, SIZE_RATIO_4_3},
    {    0,    0, SIZE_RATIO_1_1}
};

static int S5K2P6_HIGH_SPEED_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1280,  720, 0, SIZE_RATIO_16_9},
};

static int S5K2P6_FPS_RANGE_LIST[][2] =
{
    {  15000,  15000},
    {  15000,  30000},
    {  30000,  30000},
};

static int S5K2P6_HIGH_SPEED_VIDEO_FPS_RANGE_LIST[][2] =
{
    {  30000, 120000},
    { 120000, 120000},
};

/* For HAL3 : width, height, min_fps, max_fpx, vdis width, vdis height */
static int S5K2P6_AVAILABLE_VIDEO_LIST[][7] = {
#ifdef USE_UHD_RECORDING
    { 3840, 2160, 30000, 30000, 4032, 2268, 600},
#endif
#ifdef USE_WQHD_RECORDING
    { 2560, 1440, 30000, 30000, 3072, 1728, 0},
#endif
    { 1920, 1080, 60000, 60000, 2304, 1296, 600},
    { 1920, 1080, 30000, 30000, 2304, 1296, 0},
    { 1440, 1440, 30000, 30000, 0, 0, 0},
    { 1280,  720, 30000, 30000, 1536, 864, 0},
    {  640,  480, 30000, 30000, 0, 0, 0},
};

/*  vendor static info :  width, height, min_fps, max_fps, recording limit time(sec) */
static int S5K2P6_AVAILABLE_HIGH_SPEED_VIDEO_LIST[][5] = {
    { 1280, 720, 120000, 120000, 0},
};

static camera_metadata_rational UNIT_MATRIX_S5K2P6_3X3[] =
{
    {128, 128}, {0, 128}, {0, 128},
    {0, 128}, {128, 128}, {0, 128},
    {0, 128}, {0, 128}, {128, 128}
};

static camera_metadata_rational COLOR_MATRIX1_S5K2P6_3X3[] = {
    {1094, 1024}, {-306, 1024}, {-146, 1024},
    {-442, 1024}, {1388, 1024}, {52, 1024},
    {-104, 1024}, {250, 1024}, {600, 1024}
};

static camera_metadata_rational COLOR_MATRIX2_S5K2P6_3X3[] = {
    {2263, 1024}, {-1364, 1024}, {-145, 1024},
    {-194, 1024}, {1257, 1024}, {-56, 1024},
    {-24, 1024}, {187, 1024}, {618, 1024}
};

static camera_metadata_rational FORWARD_MATRIX1_S5K2P6_3X3[] = {
    {612, 1024}, {233, 1024}, {139, 1024},
    {199, 1024}, {831, 1024}, {-6, 1024},
    {15, 1024}, {-224, 1024}, {1049, 1024}
};

static camera_metadata_rational FORWARD_MATRIX2_S5K2P6_3X3[] = {
    {441, 1024}, {317, 1024}, {226, 1024},
    {29, 1024}, {908, 1024}, {87, 1024},
    {9, 1024}, {-655, 1024}, {1486, 1024}
};

#endif
