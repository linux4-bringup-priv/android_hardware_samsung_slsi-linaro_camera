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

#ifndef EXYNOS_CAMERA_LUT_2X5SP_H
#define EXYNOS_CAMERA_LUT_2X5SP_H

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

static int PREVIEW_SIZE_LUT_2X5SP[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1440p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 21:9 (Single, Dual) */
    { SIZE_RATIO_21_9,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* Dummy (not used) */
    { SIZE_RATIO_9_16,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    }
};

static int PREVIEW_SIZE_LUT_2X5SP_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.5
       BDS       = 1440p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (2880 + 0) ,(2160 + 0),   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (2880 + 0) ,(2160 + 0),   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 21:9 (Single, Dual) */
    { SIZE_RATIO_21_9,
     (2880 + 0) ,(2160 + 0),   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2880 + 0) ,(2160 + 0),   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* Dummy (not used) */
    { SIZE_RATIO_9_16,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (2880 + 0), (2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    }
};

static int PICTURE_SIZE_LUT_2X5SP[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      5760      , 4320      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      5760      , 4320      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 21:9 (Single, Dual) */
    { SIZE_RATIO_21_9,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      5760      , 4320      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      5760      , 4320      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      5760      , 4320      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      5760      , 4320      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      5760      , 4320      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      5760      , 4320      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      5760      , 4320      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* Dummy (not used) */
    { SIZE_RATIO_9_16,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      5760      , 4320      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
};

static int PICTURE_SIZE_LUT_2X5SP_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = OFF */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 21:9 (Single, Dual) */
    { SIZE_RATIO_21_9,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* Dummy (not used) */
    { SIZE_RATIO_9_16,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    }
};


static int VIDEO_SIZE_LUT_2X5SP[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1440p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5760 + 0) ,(4320 + 0),   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (5760 + 0) ,(4320 + 0),   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (5760 + 0),(4320 + 0),   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* Dummy (not used) */
    { SIZE_RATIO_9_16,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    }
};

static int VIDEO_SIZE_LUT_2X5SP_BNS[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1440p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 21:9 (Single, Dual) */
    { SIZE_RATIO_21_9,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* Dummy (not used) */
    { SIZE_RATIO_9_16,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2880      , 2160      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    }
};


static int VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2X5SP[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */

    /* HD_120 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (1920 + 0) , (1080 + 0) ,   /* [sensor ] */
      1920      ,  1080      ,   /* [bns    ] */
      1920      ,  1080      ,   /* [bcrop  ] */
      1920      ,  1080      ,   /* [bds    ] */
      1920      ,  1080      ,   /* [target ] */
    },
};

static int VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_2X5SP[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 1.0
       BDS       = ON */

    /* FHD_240 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (1280 + 0) , (720 + 0) ,   /* [sensor ] */
      1280      ,  720      ,   /* [bns    ] */
      1280      ,  720      ,   /* [bcrop  ] */
      1280      ,  720      ,   /* [bds    ] */
      1280      ,  720      ,   /* [target ] */
    },
};

static int VIDEO_SIZE_LUT_SSM_2X5SP[][SIZE_OF_LUT] =
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

static int VTCALL_SIZE_LUT_2X5SP[][SIZE_OF_LUT] =
{
    /* Binning   = 2
       BNS ratio = 1.0
       BDS       = ON */

    /* 16:9 (VT_Call) */
    { SIZE_RATIO_16_9,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2016      , 1134      ,   /* [bcrop  ] */
      2016      , 1134      ,   /* [bds    ] */
      1920      , 1080      ,   /* [target ] */
    },
    /* 4:3 (VT_Call) */
    { SIZE_RATIO_4_3,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2016      , 1512      ,   /* [bcrop  ] */
      2016      , 1512      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (VT_Call) */
    { SIZE_RATIO_1_1,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      1504      , 1504      ,   /* [bcrop  ] */
      1504      , 1504      ,   /* [bds    ] */
      1080      , 1080      ,   /* [target ] */
    },
    /* 3:2 (VT_Call) */
    { SIZE_RATIO_3_2,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      2016      , 1344      ,   /* [bcrop  ] */
      2016      , 1344      ,   /* [bds    ] */
      1616      , 1080      ,   /* [target ] */
    },
    /* 11:9 (VT_Call) */
    { SIZE_RATIO_11_9,
     (2880 + 0) ,(2160 + 0) ,   /* [sensor ] */
      2880      , 2160      ,   /* [bns    ] */
      1848      , 1512      ,   /* [bcrop  ] */
      1584      , 1296      ,   /* [bds    ] */
      1232      , 1008      ,   /* [target ] */
    },
};

static int FAST_AE_STABLE_SIZE_LUT_2X5SP[][SIZE_OF_LUT] =
{
    /* Binning   = ON
       BNS ratio = 4.0 / FPS = 120
       BDS       = ON */

    /* FAST_AE 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (1920 + 0) , (1080 + 0) ,   /* [sensor ] */
      1920      ,  1080      ,   /* [bns    ] */
      1920      ,  1080      ,   /* [bcrop  ] */
      1920      ,  1080      ,   /* [bds    ] */
      1920      ,  1080      ,   /* [target ] */
    },
};

static int PREVIEW_FULL_SIZE_LUT_2X5SP[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1440p */

    /* 16:9 (Single, Dual) */
    { SIZE_RATIO_16_9,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 4:3 (Single, Dual) */
    { SIZE_RATIO_4_3,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 21:9 (Single, Dual) */
    { SIZE_RATIO_21_9,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 3:2 (Single, Dual) */
    { SIZE_RATIO_3_2,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 5:4 (Single, Dual) */
    { SIZE_RATIO_5_4,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 5:3 (Single, Dual) */
    { SIZE_RATIO_5_3,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 11:9 (Single, Dual) */
    { SIZE_RATIO_11_9,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* Dummy (not used) */
    { SIZE_RATIO_9_16,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      2880      , 2160      ,   /* [bds    ] */
      1440      , 1080      ,   /* [target ] */
    }
};

static int PICTURE_FULL_SIZE_LUT_2X5SP[][SIZE_OF_LUT] =
{
    /* Binning   = OFF
       BNS ratio = 1.0
       BDS       = 1080p */

    /* 16:9 (Single) */
    { SIZE_RATIO_16_9,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      5760      , 4320      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 4:3 (Single) */
    { SIZE_RATIO_4_3,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      5760      , 4320      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 21:9 (Single) */
    { SIZE_RATIO_21_9,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      5760      , 4320      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 1:1 (Single, Dual) */
    { SIZE_RATIO_1_1,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      5760      , 4320      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 3:2 (Single) */
    { SIZE_RATIO_3_2,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      5760      , 4320      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 5:4 (Single) */
    { SIZE_RATIO_5_4,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      5760      , 4320      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 5:3 (Single) */
    { SIZE_RATIO_5_3,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      5760      , 4320      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 11:9 (Single) */
    { SIZE_RATIO_11_9,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      5760      , 4320      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* 18.5:9 (Single, Dual) */
    { SIZE_RATIO_18P5_9,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      5760      , 4320      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
    /* Dummy (not used) */
    { SIZE_RATIO_9_16,
     (5760 + 0) ,(4320 + 0) ,   /* [sensor ] */
      5760      , 4320      ,   /* [bns    ] */
      5760      , 4320      ,   /* [bcrop  ] */
      5760      , 4320      ,   /* [bds    ] */
      5760      , 4320      ,   /* [target ] */
    },
};

/* yuv stream size list */
static int SAK2X5SP_YUV_LIST[][SIZE_OF_RESOLUTION] =
{
    /* { width, height, minFrameDuration, ratioId } */
/*    { 5760, 4320, 33331760, SIZE_RATIO_4_3},
    { 5760, 3240, 33331760, SIZE_RATIO_16_9},
    { 5760, 2464, 33331760, SIZE_RATIO_21_9}, */
    { 2880, 2160, 33331760, SIZE_RATIO_4_3},
    { 2880, 1620, 33331760, SIZE_RATIO_16_9},
    { 2880, 1232, 33331760, SIZE_RATIO_21_9},
    { 1920, 1080, 33331760, SIZE_RATIO_16_9},
    { 1920,  816, 33331760, SIZE_RATIO_21_9},
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
static int SAK2X5SP_YUV_REPROCESSING_INPUT_LIST[][SIZE_OF_RESOLUTION] =
{
    /* { width, height, minFrameDuration, ratioId } */
/*  { 5760, 4320, 33331760, SIZE_RATIO_4_3}, */
    { 2880, 2160, 33331760, SIZE_RATIO_4_3},
};

/* yuv reprocessing input stream size list */
static int SAK2X5SP_RAW_OUTPUT_LIST[][SIZE_OF_RESOLUTION] =
{
    /* { width, height, minFrameDuration, ratioId } */
    { 2880, 2160, 33331760, SIZE_RATIO_4_3},
};

/* availble Jpeg size (only for  HAL_PIXEL_FORMAT_BLOB) */
static int SAK2X5SP_JPEG_LIST[][SIZE_OF_RESOLUTION] =
{
    /* { width, height, minFrameDuration, ratioId } */
/*    { 5760, 4320, 50000000, SIZE_RATIO_4_3},
    { 5760, 3240, 50000000, SIZE_RATIO_16_9},
    { 5760, 2464, 50000000, SIZE_RATIO_21_9}, */
    { 2880, 2160, 33331760, SIZE_RATIO_4_3},
    { 2880, 1620, 33331760, SIZE_RATIO_16_9},
    { 2880, 1232, 33331760, SIZE_RATIO_21_9},
    { 1920, 1080, 33331760, SIZE_RATIO_16_9},
    { 1920,  816, 33331760, SIZE_RATIO_21_9},
    { 1440, 1080, 33331760, SIZE_RATIO_4_3},
    { 1280,  960, 33331760, SIZE_RATIO_4_3},
    { 1280,  720, 33331760, SIZE_RATIO_16_9},
    {  960,  720, 33331760, SIZE_RATIO_4_3},
    {  800,  450, 33331760, SIZE_RATIO_16_9},
    {  640,  480, 33331760, SIZE_RATIO_4_3},
    {  352,  288, 33331760, SIZE_RATIO_11_9},
    {  320,  240, 33331760, SIZE_RATIO_4_3},
};

/* availble Jpeg size (only for  HAL_PIXEL_FORMAT_BLOB) */
static int SAK2X5SP_HIDDEN_JPEG_LIST[][SIZE_OF_RESOLUTION] =
{
    /* { width, height, minFrameDuration, ratioId } */
    { 5760, 4320, 50000000, SIZE_RATIO_4_3},
    { 5760, 3240, 50000000, SIZE_RATIO_16_9},
    { 5760, 2464, 50000000, SIZE_RATIO_21_9},
};

static int SAK2X5SP_THUMBNAIL_LIST[][SIZE_OF_RESOLUTION] =
{
    {  512,  384, 0, SIZE_RATIO_4_3},
    {  512,  288, 0, SIZE_RATIO_16_9},
    {  384,  384, 0, SIZE_RATIO_1_1},
    {  504,  216, 0, SIZE_RATIO_21_9},
    {    0,    0, 0, SIZE_RATIO_1_1},
};

/* For HAL3 */
static int SAK2X5SP_HIGH_SPEED_VIDEO_LIST[][SIZE_OF_RESOLUTION] =
{
    { 1920,  1080, 120, SIZE_RATIO_16_9},
    { 1280,  720, 240, SIZE_RATIO_16_9},
};

static int SAK2X5SP_FPS_RANGE_LIST[][2] =
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

/* For HAL3 */
static int SAK2X5SP_HIGH_SPEED_VIDEO_FPS_RANGE_LIST[][2] =
{
    { 30000, 120000},
    { 120000, 120000},
    { 30000, 240000},
    { 240000, 240000},
};

/* vendor static info : width, height, min_fps, max_fps, vdis width, vdis height, recording limit time(sec) */
static int SAK2X5SP_AVAILABLE_VIDEO_LIST[][7] =
{
    { 1920, 1080, 30000, 30000, 2304, 1296, 0},
    { 1920,  816, 30000, 30000, 2304, 980, 0},
    { 1280,  720, 30000, 30000, 1536, 864, 0},
    {  640,  480, 30000, 30000, 0, 0, 0},
    {  320,  240, 30000, 30000, 0, 0, 0},    /* For support the CameraProvider lib of Message app*/
    {  176,  144, 30000, 30000, 0, 0, 0},    /* For support the CameraProvider lib of Message app*/
};

/*  vendor static info :  width, height, min_fps, max_fps, recording limit time(sec) */
static int SAK2X5SP_AVAILABLE_HIGH_SPEED_VIDEO_LIST[][5] =
{
    { 1920, 1080, 120000, 120000, 0},
    { 1280, 720, 240000, 240000, 0},
};

/* effect fps range */
static int SAK2X5SP_EFFECT_FPS_RANGE_LIST[][2] =
{
    {  10000,  24000},
    {  24000,  24000},
};

static camera_metadata_rational COLOR_MATRIX1_2X5SP_3X3[] =
{
    {661, 1024}, {-62, 1024}, {-110, 1024},
    {-564, 1024}, {1477, 1024}, {77, 1024},
    {-184, 1024}, {445, 1024}, {495, 1024}
};

static camera_metadata_rational COLOR_MATRIX2_2X5SP_3X3[] =
{
    {1207, 1024}, {-455, 1024}, {-172, 1024},
    {-488, 1024}, {1522, 1024}, {107, 1024},
    {-82, 1024}, {314, 1024}, {713, 1024}
};

static camera_metadata_rational FORWARD_MATRIX1_2X5SP_3X3[] =
{
    {759, 1024}, {5, 1024}, {223, 1024},
    {292, 1024}, {732, 1024}, {0, 1024},
    {13, 1024}, {-494, 1024}, {1325, 1024}
};

static camera_metadata_rational FORWARD_MATRIX2_2X5SP_3X3[] =
{
    {655, 1024}, {68, 1024}, {265, 1024},
    {186, 1024}, {810, 1024}, {28, 1024},
    {-34, 1024}, {-821, 1024}, {1700, 1024}
};
#endif
