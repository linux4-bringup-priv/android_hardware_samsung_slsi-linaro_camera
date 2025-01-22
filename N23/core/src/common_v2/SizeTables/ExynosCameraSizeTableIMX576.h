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

#ifndef EXYNOS_CAMERA_LUT_IMX576_H
#define EXYNOS_CAMERA_LUT_IMX576_H

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
    Sensor Margin Width  = 16,
    Sensor Margin Height = 10
-----------------------------*/

static int PREVIEW_SIZE_LUT_IMX576[][SIZE_OF_LUT] =
{
    /* Binning   = 1
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5760 + 0) ,(3240 + 0) ,   /* [sensor ] */
      5760      , 3240      ,   /* [bns    ] */
      5760      , 3240      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (5760 + 0) ,(4312 + 0) ,   /* [sensor ] */
      5760      , 4312      ,   /* [bns    ] */
      5760      , 4312      ,   /* [bcrop  ] */
      2880      , 2156      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4312 + 0) ,(4312 + 0) ,   /* [sensor ] */
      4312      , 4312      ,   /* [bns    ] */
      4312      , 4312      ,   /* [bcrop  ] */
      2156      , 2156      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5664      , 3776      ,   /* [bcrop  ] */
      5664      , 3776      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5300      , 4240      ,   /* [bcrop  ] */
      5300      , 4240      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5660      , 3396      ,   /* [bcrop  ] */
      5660      , 3396      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5192      , 4248      ,   /* [bcrop  ] */
      5192      , 4248      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] */
    },
    /*  Dummy (not used) */
    { SIZE_RATIO_9_16,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5192      , 4248      ,   /* [bcrop  ] */
      5192      , 4248      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (5760 + 0) ,(2800 + 0) ,   /* [sensor ] */
      5760      , 2800      ,   /* [bns    ] */
      5760      , 2800      ,   /* [bcrop  ] */
      2880      , 1400      ,   /* [bds    ] */
      2224      , 1080      ,   /* [target ] */
    },
};

static int PREVIEW_SIZE_LUT_IMX576_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = 2
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (2832 + 0) ,(1592 + 0) ,   /* [sensor ] */
      2832      , 1592      ,   /* [bns    ] */
      2832      , 1592      ,   /* [bcrop  ] */
      2832      , 1592      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (2832 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2832      , 2124      ,   /* [bns    ] */
      2832      , 2124      ,   /* [bcrop  ] */
      2832      , 2124      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2124 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2124      , 2124      ,   /* [bns    ] */
      2124      , 2124      ,   /* [bcrop  ] */
      2124      , 2124      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (2832 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2832      , 2124      ,   /* [bns    ] */
      2832      , 1888      ,   /* [bcrop  ] */
      2832      , 1888      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (2832 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2832      , 2124      ,   /* [bns    ] */
      2640      , 2112      ,   /* [bcrop  ] */
      2640      , 2112      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (2832 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2832      , 2124      ,   /* [bns    ] */
      2830      , 1698      ,   /* [bcrop  ] */
      2830      , 1698      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (2832 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2832      , 2124      ,   /* [bns    ] */
      2596      , 2124      ,   /* [bcrop  ] */
      2596      , 2124      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] */
    },
    /*  Dummy (not used) */
    { SIZE_RATIO_9_16,
     (2832 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2832      , 2124      ,   /* [bns    ] */
      2596      , 2124      ,   /* [bcrop  ] */
      2596      , 2124      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (2832 + 0) ,(1376 + 0) ,   /* [sensor ] */
      2832      , 1376      ,   /* [bns    ] */
      2832      , 1376      ,   /* [bcrop  ] */
      2832      , 1376      ,   /* [bds    ] */
      2224      , 1080      ,   /* [target ] */
    },
};

static int PICTURE_SIZE_LUT_IMX576[][SIZE_OF_LUT] =
{
    /* Binning   = 1
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5664 + 0) ,(3184 + 0) ,   /* [sensor ] */
      5664      , 3184      ,   /* [bns    ] */
      5664      , 3184      ,   /* [bcrop  ] */
      5664      , 3184      ,   /* [bds    ] */
      5664      , 3184      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5664      , 4248      ,   /* [bcrop  ] */
      5664      , 4248      ,   /* [bds    ] */
      5664      , 4248      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4248 + 0) ,(4248 + 0) ,   /* [sensor ] */
      4248      , 4248      ,   /* [bns    ] */
      4248      , 4248      ,   /* [bcrop  ] */
      4248      , 4248      ,   /* [bds    ] */
      4248      , 4248      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5664      , 3776      ,   /* [bcrop  ] */
      5664      , 3776      ,   /* [bds    ] */
      5664      , 3776      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5300      , 4240      ,   /* [bcrop  ] */
      5300      , 4240      ,   /* [bds    ] */
      5300      , 4240      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5660      , 3396      ,   /* [bcrop  ] */
      5660      , 3396      ,   /* [bds    ] */
      5660      , 3396      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5192      , 4248      ,   /* [bcrop  ] */
      5192      , 4248      ,   /* [bds    ] */
      5192      , 4248      ,   /* [target ] */
    },
    /*  Dummy (not used) */
    { SIZE_RATIO_9_16,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5192      , 4248      ,   /* [bcrop  ] */
      5192      , 4248      ,   /* [bds    ] */
      5192      , 4248      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (5664 + 0) ,(2752 + 0) ,   /* [sensor ] */
      5664      , 2752      ,   /* [bns    ] */
      5664      , 2752      ,   /* [bcrop  ] */
      5664      , 2752      ,   /* [bds    ] */
      5664      , 2752      ,   /* [target ] */
    },
};

static int PICTURE_SIZE_LUT_IMX576_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = 2
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (2832 + 0) ,(1592 + 0) ,   /* [sensor ] */
      2832      , 1592      ,   /* [bns    ] */
      2832      , 1592      ,   /* [bcrop  ] */
      2832      , 1592      ,   /* [bds    ] */
      5664      , 3184      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (2832 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2832      , 2124      ,   /* [bns    ] */
      2832      , 2124      ,   /* [bcrop  ] */
      2832      , 2124      ,   /* [bds    ] */
      5664      , 4248      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2124 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2124      , 2124      ,   /* [bns    ] */
      2124      , 2124      ,   /* [bcrop  ] */
      2124      , 2124      ,   /* [bds    ] */
      4248      , 4248      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (2832 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2832      , 2124      ,   /* [bns    ] */
      2832      , 1888      ,   /* [bcrop  ] */
      2832      , 1888      ,   /* [bds    ] */
      5664      , 3776      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (2832 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2832      , 2124      ,   /* [bns    ] */
      2640      , 2112      ,   /* [bcrop  ] */
      2640      , 2112      ,   /* [bds    ] */
      5300      , 4240      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (2832 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2832      , 2124      ,   /* [bns    ] */
      2830      , 1698      ,   /* [bcrop  ] */
      2830      , 1698      ,   /* [bds    ] */
      5660      , 3396      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (2832 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2832      , 2124      ,   /* [bns    ] */
      2596      , 2124      ,   /* [bcrop  ] */
      2596      , 2124      ,   /* [bds    ] */
      5192      , 4248      ,   /* [target ] */
    },
    /*  Dummy (not used) */
    { SIZE_RATIO_9_16,
     (2832 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2832      , 2124      ,   /* [bns    ] */
      2596      , 2124      ,   /* [bcrop  ] */
      2596      , 2124      ,   /* [bds    ] */
      5192      , 4248      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (2832 + 0) ,(1376 + 0) ,   /* [sensor ] */
      2832      , 1376      ,   /* [bns    ] */
      2832      , 1376      ,   /* [bcrop  ] */
      2832      , 1376      ,   /* [bds    ] */
      5664      , 2752      ,   /* [target ] */
    },
};

static int VIDEO_SIZE_LUT_IMX576_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = 2
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (2832 + 0) ,(1592 + 0) ,   /* [sensor ] */
      2832      , 1592      ,   /* [bns    ] */
      2832      , 1592      ,   /* [bcrop  ] */
      2832      , 1592      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (2832 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2832      , 2124      ,   /* [bns    ] */
      2832      , 2124      ,   /* [bcrop  ] */
      2832      , 2124      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2124 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2124      , 2124      ,   /* [bns    ] */
      2124      , 2124      ,   /* [bcrop  ] */
      2124      , 2124      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (2832 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2832      , 2124      ,   /* [bns    ] */
      2832      , 1888      ,   /* [bcrop  ] */
      2832      , 1888      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (2832 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2832      , 2124      ,   /* [bns    ] */
      2640      , 2112      ,   /* [bcrop  ] */
      2640      , 2112      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (2832 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2832      , 2124      ,   /* [bns    ] */
      2830      , 1698      ,   /* [bcrop  ] */
      2830      , 1698      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (2832 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2832      , 2124      ,   /* [bns    ] */
      2596      , 2124      ,   /* [bcrop  ] */
      2596      , 2124      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] */
    },
    /*  Dummy (not used) */
    { SIZE_RATIO_9_16,
     (2832 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2832      , 2124      ,   /* [bns    ] */
      2596      , 2124      ,   /* [bcrop  ] */
      2596      , 2124      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (2832 + 0) ,(1376 + 0) ,   /* [sensor ] */
      2832      , 1376      ,   /* [bns    ] */
      2832      , 1376      ,   /* [bcrop  ] */
      2832      , 1376      ,   /* [bds    ] */
      2224      , 1080      ,   /* [target ] */
    },
};

static int VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX576[][SIZE_OF_LUT] =
{
    /* Binning   = 2
       BNS ratio = 1.0
       BDS       = OFF */

    /*   FHD_120  16:9 (Single) */
    { SIZE_RATIO_16_9,
     (1920 + 0) ,(1080 + 0) ,   /* [sensor ] */
      1920      , 1080      ,   /* [bns    ] */
      1920      , 1080      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
};

static int VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_IMX576[][SIZE_OF_LUT] =
{
    /* Binning   = 2
       BNS ratio = 1.0
       BDS       = OFF */

    /*   SuperSlowMotion_240FPS  16:9 (Single) */
    { SIZE_RATIO_16_9,
     (1872 + 0) ,(1052 + 0) ,   /* [sensor ] */
      1872      , 1052      ,   /* [bns    ] */
      1872      , 1052      ,   /* [bcrop  ] */
      1872      , 1052      ,   /* [bds    ] */
      1872      , 1052      ,   /* [target ] */
    },
    /*   SuperSlowMotion_240FPS  16:9 (Single) */
    { SIZE_RATIO_16_9,
     (1280 + 0) ,(720  + 0) ,   /* [sensor ] */
      1280      , 720       ,   /* [bns    ] */
      1280      , 720       ,   /* [bcrop  ] */
      1280      , 720       ,   /* [bds    ] */
      1280      , 720       ,   /* [target ] */
    },
};

static int FAST_AE_STABLE_SIZE_LUT_IMX576[][SIZE_OF_LUT] =
{
    /* Binning   = 2
       BNS ratio = 1.0 / FPS = 120
       BDS       = OFF */

    /*   FHD_120  16:9 (Single) */
    { SIZE_RATIO_16_9,
     (1920 + 0) ,(1080 + 0) ,   /* [sensor ] */
      1920      , 1080      ,   /* [bns    ] */
      1920      , 1080      ,   /* [bcrop  ] */
      1920      , 1080      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
};

static int VTCALL_SIZE_LUT_IMX576[][SIZE_OF_LUT] =
{
    /* Binning   = 2
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (VT_Call) */
    { SIZE_RATIO_16_9,
     (2832 + 0) ,(1592 + 0 ),   /* [sensor ] */
      2832      , 1592      ,   /* [bns    ] */
      2832      , 1592      ,   /* [bcrop  ] */
      2832      , 1592      ,   /* [bds    ] */
      1280      , 720       ,   /* [target ] */
    },
    /*  4:3 (VT_Call) */
    { SIZE_RATIO_4_3,
     (2832 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2832      , 2124      ,   /* [bns    ] */
      2832      , 2124      ,   /* [bcrop  ] */
      2832      , 2124      ,   /* [bds    ] */
      960       , 720       ,   /* [target ] */
    },
    /*  1:1 (VT_Call) */
    { SIZE_RATIO_1_1,
     (2124 + 0) ,(2124 + 0) ,   /* [sensor ] */
      2124      , 2124      ,   /* [bns    ] */
      2124      , 2124      ,   /* [bcrop  ] */
      2124      , 2124      ,   /* [bds    ] */
      720       , 720       ,   /* [target ] */
    },
};

static int PREVIEW_FULL_SIZE_LUT_IMX576[][SIZE_OF_LUT] =
{
    /* Binning   = 1
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5664 + 0) ,(3184 + 0) ,   /* [sensor ] */
      5664      , 3184      ,   /* [bns    ] */
      5664      , 3184      ,   /* [bcrop  ] */
      5664      , 3184      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5664      , 4248      ,   /* [bcrop  ] */
      5664      , 4248      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4248 + 0) ,(4248 + 0) ,   /* [sensor ] */
      4248      , 4248      ,   /* [bns    ] */
      4248      , 4248      ,   /* [bcrop  ] */
      4248      , 4248      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5664      , 3776      ,   /* [bcrop  ] */
      5664      , 3776      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5300      , 4240      ,   /* [bcrop  ] */
      5300      , 4240      ,   /* [bds    ] */
      1344      , 1080      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5660      , 3396      ,   /* [bcrop  ] */
      5660      , 3396      ,   /* [bds    ] */
      1792      , 1080      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5192      , 4248      ,   /* [bcrop  ] */
      5192      , 4248      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] */
    },
    /*  Dummy (not used) */
    { SIZE_RATIO_9_16,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5192      , 4248      ,   /* [bcrop  ] */
      5192      , 4248      ,   /* [bds    ] */
      1312      , 1080      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (5664 + 0) ,(2752 + 0) ,   /* [sensor ] */
      5664      , 2752      ,   /* [bns    ] */
      5664      , 2752      ,   /* [bcrop  ] */
      5664      , 2752      ,   /* [bds    ] */
      2224      , 1080      ,   /* [target ] */
    },
};

static int PICTURE_FULL_SIZE_LUT_IMX576[][SIZE_OF_LUT] =
{
    /* Binning   = 1
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5664 + 0) ,(3184 + 0) ,   /* [sensor ] */
      5664      , 3184      ,   /* [bns    ] */
      5664      , 3184      ,   /* [bcrop  ] */
      5664      , 3184      ,   /* [bds    ] */
      5664      , 3184      ,   /* [target ] */
    },
    /*  4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5664      , 4248      ,   /* [bcrop  ] */
      5664      , 4248      ,   /* [bds    ] */
      5664      , 4248      ,   /* [target ] */
    },
    /*  1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (4248 + 0) ,(4248 + 0) ,   /* [sensor ] */
      4248      , 4248      ,   /* [bns    ] */
      4248      , 4248      ,   /* [bcrop  ] */
      4248      , 4248      ,   /* [bds    ] */
      4248      , 4248      ,   /* [target ] */
    },
    /*  3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5664      , 3776      ,   /* [bcrop  ] */
      5664      , 3776      ,   /* [bds    ] */
      5664      , 3776      ,   /* [target ] */
    },
    /*  5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5300      , 4240      ,   /* [bcrop  ] */
      5300      , 4240      ,   /* [bds    ] */
      5300      , 4240      ,   /* [target ] */
    },
    /*  5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5660      , 3396      ,   /* [bcrop  ] */
      5660      , 3396      ,   /* [bds    ] */
      5660      , 3396      ,   /* [target ] */
    },
    /*  11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5192      , 4248      ,   /* [bcrop  ] */
      5192      , 4248      ,   /* [bds    ] */
      5192      , 4248      ,   /* [target ] */
    },
    /*  Dummy (not used) */
    { SIZE_RATIO_9_16,
     (5664 + 0) ,(4248 + 0) ,   /* [sensor ] */
      5664      , 4248      ,   /* [bns    ] */
      5192      , 4248      ,   /* [bcrop  ] */
      5192      , 4248      ,   /* [bds    ] */
      5192      , 4248      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (5664 + 0) ,(2752 + 0) ,   /* [sensor ] */
      5664      , 2752      ,   /* [bns    ] */
      5664      , 2752      ,   /* [bcrop  ] */
      5664      , 2752      ,   /* [bds    ] */
      5664      , 2752      ,   /* [target ] */
    },
};

static int IMX576_YUV_LIST[][SIZE_OF_RESOLUTION] =
{
    { 5664, 4248, 33331760, SIZE_RATIO_4_3},
    { 5664, 3184, 33331760, SIZE_RATIO_16_9},
    { 4248, 4248, 33331760, SIZE_RATIO_1_1},
    { 3840, 2160, 33331760, SIZE_RATIO_16_9},
    { 3264, 2448, 33331760, SIZE_RATIO_4_3},
    { 3264, 1836, 33331760, SIZE_RATIO_16_9},
    { 2832, 2124, 33331760, SIZE_RATIO_4_3},
    { 2160, 2160, 33331760, SIZE_RATIO_1_1},
    { 1920, 1080, 33331760, SIZE_RATIO_16_9},
    { 1440, 1080, 16665880, SIZE_RATIO_4_3},
    { 1080, 1080, 16665880, SIZE_RATIO_1_1},
    { 1024,  768, 16665880, SIZE_RATIO_4_3},
    { 1280,  720, 16665880, SIZE_RATIO_16_9},
    {  960,  720, 16665880, SIZE_RATIO_4_3},
    { 1056,  704, 16665880, SIZE_RATIO_3_2},
    {  800,  450, 16665880, SIZE_RATIO_16_9},
    {  720,  720, 16665880, SIZE_RATIO_1_1},
    {  720,  480, 16665880, SIZE_RATIO_3_2},
    {  640,  480, 16665880, SIZE_RATIO_4_3},
    {  480,  320, 16665880, SIZE_RATIO_3_2},
    {  352,  288, 16665880, SIZE_RATIO_11_9},
    {  320,  240, 16665880, SIZE_RATIO_4_3},
    {  256,  144, 16665880, SIZE_RATIO_16_9}, /* DngCreatorTest */
    {  176,  144, 16665880, SIZE_RATIO_11_9}, /* RecordingTest */
};

static int IMX576_HIDDEN_PREVIEW_LIST[][SIZE_OF_RESOLUTION] =
{
    { 5664, 4248, 33331760, SIZE_RATIO_4_3},
    { 5664, 3184, 33331760, SIZE_RATIO_16_9},
    { 4248, 4248, 33331760, SIZE_RATIO_1_1},
    { 3840, 2160, 33331760, SIZE_RATIO_16_9},
    { 3264, 2448, 33331760, SIZE_RATIO_4_3},
    { 3264, 1836, 33331760, SIZE_RATIO_16_9},
    { 2832, 2124, 33331760, SIZE_RATIO_4_3},
    { 2160, 2160, 33331760, SIZE_RATIO_1_1},
    { 1920, 1080, 33331760, SIZE_RATIO_16_9},
    { 1440, 1080, 16665880, SIZE_RATIO_4_3},
    { 1080, 1080, 16665880, SIZE_RATIO_1_1},
    { 1024,  768, 16665880, SIZE_RATIO_4_3},
    { 1280,  720, 16665880, SIZE_RATIO_16_9},
    {  960,  720, 16665880, SIZE_RATIO_4_3},
    { 1056,  704, 16665880, SIZE_RATIO_3_2},
    {  800,  450, 16665880, SIZE_RATIO_16_9},
    {  720,  720, 16665880, SIZE_RATIO_1_1},
    {  720,  480, 16665880, SIZE_RATIO_3_2},
    {  640,  480, 16665880, SIZE_RATIO_4_3},
    {  480,  320, 16665880, SIZE_RATIO_3_2},
    {  352,  288, 16665880, SIZE_RATIO_11_9},
    {  320,  240, 16665880, SIZE_RATIO_4_3},
};

static int IMX576_JPEG_LIST[][SIZE_OF_RESOLUTION] =
{
    { 5664, 4248, 50000000, SIZE_RATIO_4_3},
    { 5664, 3184, 50000000, SIZE_RATIO_16_9},
    { 4608, 3456, 50000000, SIZE_RATIO_4_3},
    { 4248, 4248, 50000000, SIZE_RATIO_1_1},
    { 3264, 2448, 50000000, SIZE_RATIO_4_3},
    { 3264, 1836, 50000000, SIZE_RATIO_16_9},
    { 2448, 2448, 50000000, SIZE_RATIO_1_1},
    { 2048, 1152, 50000000, SIZE_RATIO_16_9},
    { 1920, 1080, 33331760, SIZE_RATIO_16_9},
    { 1280,  720, 33331760, SIZE_RATIO_16_9},
    {  960,  720, 33331760, SIZE_RATIO_4_3},
    {  640,  480, 33331760, SIZE_RATIO_4_3},
    {  320,  240, 33331760, SIZE_RATIO_4_3},  /* for cts2 testAvailableStreamConfigs */
    {  256,  144, 33331760, SIZE_RATIO_16_9}, /*  for cts2 testSingleImageThumbnail */
};

static int IMX576_HIDDEN_PICTURE_LIST[][SIZE_OF_RESOLUTION] =
{
    { 5664, 2752, 50000000, SIZE_RATIO_18P5_9},
    { 4128, 3096, 50000000, SIZE_RATIO_4_3},
    { 4128, 2322, 50000000, SIZE_RATIO_16_9},
    { 4096, 3072, 50000000, SIZE_RATIO_4_3},
    { 4096, 2304, 50000000, SIZE_RATIO_16_9},
    { 4032, 3024, 50000000, SIZE_RATIO_4_3},
    { 4032, 2268, 50000000, SIZE_RATIO_16_9},
    { 4032, 1960, 50000000, SIZE_RATIO_18P5_9},
    { 3840, 2160, 50000000, SIZE_RATIO_16_9},
    { 3456, 2592, 50000000, SIZE_RATIO_4_3},
    { 3200, 2400, 50000000, SIZE_RATIO_4_3},
    { 3072, 1728, 50000000, SIZE_RATIO_16_9},
    { 2988, 2988, 50000000, SIZE_RATIO_1_1},
    { 2592, 2592, 50000000, SIZE_RATIO_1_1},
    { 2592, 1944, 50000000, SIZE_RATIO_4_3},
    { 2592, 1936, 50000000, SIZE_RATIO_4_3},  /* not exactly matched ratio */
    { 2560, 1920, 50000000, SIZE_RATIO_4_3},
    { 2048, 1536, 50000000, SIZE_RATIO_4_3},
    { 1920, 1440, 50000000, SIZE_RATIO_4_3},  /* For WideSelfie Shot */
    { 1440, 1080, 33331760, SIZE_RATIO_4_3},  /* For WideSelfie Shot */
    { 1280,  960, 33331760, SIZE_RATIO_4_3},  /* for VtCamera Test*/
    {  960,  540, 33331760, SIZE_RATIO_16_9},
    {  720,  720, 33331760, SIZE_RATIO_1_1},  /* dummy size for binning mode */
    {  352,  288, 33331760, SIZE_RATIO_11_9}, /* dummy size for binning mode */
};

static int IMX576_HIGH_SPEED_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1280,  720, 0, SIZE_RATIO_16_9},
};

static int IMX576_HIGH_SPEED_VIDEO_FPS_RANGE_LIST[][2] =
{
    {  30000, 120000},
    {  60000, 120000},
    { 120000, 120000},
};

static int IMX576_FPS_RANGE_LIST[][2] =
{
    {  15000,  15000},
    {  24000,  24000},
    {   8000,  30000},
    {  10000,  30000},
    {  15000,  30000},
    {  30000,  30000},
};

static int IMX576_HIDDEN_FPS_RANGE_LIST[][2] =
{
    {   8000,  24000},
    {  10000,  24000},
    {  30000,  60000},
    {  60000,  60000},
    {  60000, 120000},
    { 120000, 120000},
    {  60000, 240000},
    { 120000, 240000},
    { 240000, 240000},
};

#endif
