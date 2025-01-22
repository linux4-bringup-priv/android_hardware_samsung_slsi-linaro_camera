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

#ifndef EXYNOS_CAMERA_LUT_5E9_H
#define EXYNOS_CAMERA_LUT_5E9_H

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

static int PREVIEW_SIZE_LUT_5E9[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF  */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (2592 + 0) ,(1944 + 0) ,   /* [sensor ] */
      2592      , 1944      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      2560      , 1920      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (2592 + 0) ,(1944 + 0) ,   /* [sensor ] */
      2592      , 1944      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      2560      , 1920      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (2592 + 0) ,(1944 + 0) ,   /* [sensor ] */
      2592      , 1944      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      2560      , 1920      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (2592 + 0) ,(1944 + 0) ,   /* [sensor ] */
      2592      , 1944      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      2560      , 1920      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    }
};

static int PICTURE_SIZE_LUT_5E9[][SIZE_OF_LUT] =
{
    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (2592 + 0) ,(1944 + 0) ,   /* [sensor ] */
      2592      , 1944      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      2592      , 1944      ,   /* [bds    ] */
      2592      , 1944      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (2592 + 0) ,(1944 + 0) ,   /* [sensor ] */
      2592      , 1944      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      2592      , 1944      ,   /* [bds    ] */
      2592      , 1944      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (2592 + 0) ,(1944 + 0) ,   /* [sensor ] */
      2592      , 1944      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      2592      , 1944      ,   /* [bds    ] */
      2592      , 1944      ,   /* [target ] */
    },
};

static int VIDEO_SIZE_LUT_5E9[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF  */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (2592 + 0) ,(1944 + 0) ,   /* [sensor ] */
      2592      , 1944      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      2560      , 1920      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (2592 + 0) ,(1944 + 0) ,   /* [sensor ] */
      2592      , 1944      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      2560      , 1920      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (2592 + 0) ,(1944 + 0) ,   /* [sensor ] */
      2592      , 1944      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      2560      , 1920      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    }
};

static int VTCALL_SIZE_LUT_5E9[][SIZE_OF_LUT] =
{
};

static int FAST_AE_STABLE_SIZE_LUT_5E9[][SIZE_OF_LUT] =
{
};

static int PREVIEW_FULL_SIZE_LUT_5E9[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF  */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (2592 + 0) ,(1944 + 0) ,   /* [sensor ] */
      2592      , 1944      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      2560      , 1920      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (2592 + 0) ,(1944 + 0) ,   /* [sensor ] */
      2592      , 1944      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      2560      , 1920      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (2592 + 0) ,(1944 + 0) ,   /* [sensor ] */
      2592      , 1944      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      2560      , 1920      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (2592 + 0) ,(1944 + 0) ,   /* [sensor ] */
      2592      , 1944      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      2560      , 1920      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    }
};

static int PICTURE_FULL_SIZE_LUT_5E9[][SIZE_OF_LUT] =
{
    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (2592 + 0) ,(1944 + 0) ,   /* [sensor ] */
      2592      , 1944      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      2592      , 1944      ,   /* [bds    ] */
      2592      , 1944      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (2592 + 0) ,(1944 + 0) ,   /* [sensor ] */
      2592      , 1944      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      2592      , 1944      ,   /* [bds    ] */
      2592      , 1944      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (2592 + 0) ,(1944 + 0) ,   /* [sensor ] */
      2592      , 1944      ,   /* [bns    ] */
      2592      , 1944      ,   /* [bcrop  ] */
      2592      , 1944      ,   /* [bds    ] */
      2592      , 1944      ,   /* [target ] */
    },
};

static int S5K5E9_YUV_LIST[][SIZE_OF_RESOLUTION] =
{
    { 2592, 1944, 33331760, SIZE_RATIO_4_3},
    { 2592, 1458, 33331760, SIZE_RATIO_16_9},
    { 2560, 1920, 33331760, SIZE_RATIO_4_3},
    { 2560, 1440, 33331760, SIZE_RATIO_16_9},
    { 1920, 1080, 33331760, SIZE_RATIO_16_9},
    { 1440, 1080, 33331760, SIZE_RATIO_4_3},
    { 1280,  960, 33331760, SIZE_RATIO_4_3},
    { 1280,  720, 33331760, SIZE_RATIO_16_9},
    {  960,  720, 33331760, SIZE_RATIO_4_3},
    {  720,  480, 33331760, SIZE_RATIO_3_2},
    {  640,  480, 33331760, SIZE_RATIO_4_3},
    {  320,  240, 33331760, SIZE_RATIO_4_3},
    {  256,  144, 33331760, SIZE_RATIO_16_9},
    {  176,  144, 33331760, SIZE_RATIO_11_9},
};

/* yuv reprocessing input stream size list */
static int S5K5E9_YUV_REPROCESSING_INPUT_LIST[][SIZE_OF_RESOLUTION] =
{
    { 2592, 1944, 33331760, SIZE_RATIO_4_3},
};

/* Raw output stream size list */
static int S5K5E9_RAW_OUTPUT_LIST[][SIZE_OF_RESOLUTION] =
{
    { 2592, 1944, 33331760, SIZE_RATIO_4_3},
};

static int S5K5E9_JPEG_LIST[][SIZE_OF_RESOLUTION] =
{
    { 2592, 1944, 33331760, SIZE_RATIO_4_3},
    { 2560, 1920, 33331760, SIZE_RATIO_4_3},
    { 2560, 1440, 33331760, SIZE_RATIO_16_9},
    { 2592, 1458, 33331760, SIZE_RATIO_16_9},
    { 1920, 1080, 33331760, SIZE_RATIO_16_9},
    { 1440, 1080, 33331760, SIZE_RATIO_4_3},
    { 1280,  960, 33331760, SIZE_RATIO_4_3},
    { 1280,  720, 33331760, SIZE_RATIO_16_9},
    {  960,  720, 33331760, SIZE_RATIO_4_3},
    {  720,  480, 33331760, SIZE_RATIO_3_2},
    {  640,  480, 33331760, SIZE_RATIO_4_3},
    {  320,  240, 33331760, SIZE_RATIO_4_3},
};

static int S5K5E9_HIGH_SPEED_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
};

static int S5K5E9_FPS_RANGE_LIST[][2] =
{
    {   7000,   7000},
    {   7000,  15000},
    {  15000,  15000},
    {   7000,  20000},
    {   7000,  24000},
    {  24000,  24000},
    {   8000,  30000},
    {  10000,  30000},
    {  15000,  30000},
    {  24000,  30000},
    {  30000,  30000},
};

static int S5K5E9_HIGH_SPEED_VIDEO_FPS_RANGE_LIST[][2] =
{
};

static int S5K5E9_AVAILABLE_HIGH_SPEED_VIDEO_LIST[][5] =
{
};

static camera_metadata_rational COLOR_MATRIX1_5E9_3X3[] =
{
    {661, 1024}, {-62, 1024}, {-110, 1024},
    {-564, 1024}, {1477, 1024}, {77, 1024},
    {-184, 1024}, {445, 1024}, {495, 1024}
};

static camera_metadata_rational COLOR_MATRIX2_5E9_3X3[] =
{
    {1207, 1024}, {-455, 1024}, {-172, 1024},
    {-488, 1024}, {1522, 1024}, {107, 1024},
    {-82, 1024}, {314, 1024}, {713, 1024}
};

static camera_metadata_rational FORWARD_MATRIX1_5E9_3X3[] =
{
    {759, 1024}, {5, 1024}, {223, 1024},
    {292, 1024}, {732, 1024}, {0, 1024},
    {13, 1024}, {-494, 1024}, {1325, 1024}
};

static camera_metadata_rational FORWARD_MATRIX2_5E9_3X3[] =
{
    {655, 1024}, {68, 1024}, {265, 1024},
    {186, 1024}, {810, 1024}, {28, 1024},
    {-34, 1024}, {-821, 1024}, {1700, 1024}
};

#endif
