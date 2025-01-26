/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/*#define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraPP"

#include "ExynosCameraPP.h"

ExynosCameraPP::~ExynosCameraPP()
{
    SAFE_DELETE(m_nextPP);
}

status_t ExynosCameraPP::create(void)
{
    status_t ret = NO_ERROR;

    Mutex::Autolock lock(m_lock);

    if (this->m_flagCreated() == true) {
        CLOGE("It is already created. so, fail");
        return INVALID_OPERATION;
    }

    ret = ExynosCameraPP::m_create();
    if (ret != NO_ERROR) {
        CLOGE("ExynosCameraPP::m_create() fail");
        return INVALID_OPERATION;
    }

    ret = this->m_create();
    if (ret != NO_ERROR) {
        CLOGE("this->m_create() fail");
        return INVALID_OPERATION;
    }

    m_flagCreate = true;

    CLOGD("done");

    return ret;
}

status_t ExynosCameraPP::extControl(int controlType, void *data)
{
    status_t ret = NO_ERROR;

    Mutex::Autolock lock(m_lock);

    ret = ExynosCameraPP::m_extControl(controlType, data);
    if (ret != NO_ERROR) {
        CLOGE("ExynosCameraPP::m_extControl() fail");
        return INVALID_OPERATION;
    }

    ret = this->m_extControl(controlType, data);
    if (ret != NO_ERROR) {
        CLOGE("this->m_extControl() fail");
        return INVALID_OPERATION;
    }

    CLOGD("done %d", controlType);

    return ret;
}

status_t ExynosCameraPP::destroy(void)
{
    status_t ret = NO_ERROR;
    status_t funcRet = NO_ERROR;

    Mutex::Autolock lock(m_lock);

    if (this->m_flagCreated() == false) {
        CLOGE("It is not created. so, fail");
        return INVALID_OPERATION;
    }

    ret = ExynosCameraPP::m_destroy();
    funcRet |= ret;
    if (ret != NO_ERROR) {
        CLOGE("ExynosCameraPP::m_destroy() fail");
    }

    ret = this->m_destroy();
    funcRet |= ret;
    if (ret != NO_ERROR) {
        CLOGE("this->m_destroy() fail");
    }

    /*
     * destroy about nextPP.
     * we don't need to call destroy() of m_nextPP's m_nextPP.
     * because it will call destroy() automatically, reculsively.
     */
    if (m_nextPP != NULL &&
        m_nextPP->m_flagCreated() == true) {
        ret = m_nextPP->destroy();
        funcRet |= ret;
        if (ret != NO_ERROR) {
            CLOGE("m_nextPP(%s)->destroy() fail", m_nextPP->getName());
        }
    }

    m_flagCreate = false;

    CLOGD("done");

    return funcRet;
}

bool ExynosCameraPP::flagCreated(void)
{
    Mutex::Autolock lock(m_lock);

    return m_flagCreated();
}

int ExynosCameraPP::getNodeNum(void)
{
    return m_nodeNum;
}

ExynosCameraImageCapacity ExynosCameraPP::getSrcImageCapacity(void)
{
    return m_srcImageCapacity;
}

ExynosCameraImageCapacity ExynosCameraPP::getDstImageCapacity(void)
{
    return m_dstImageCapacity;
}

int ExynosCameraPP::getNumOfSrcImage(void)
{
    return m_srcImageCapacity.getNumOfImage();
}

int ExynosCameraPP::getNumOfDstImage(void)
{
    return m_dstImageCapacity.getNumOfImage();
}

bool ExynosCameraPP::isSupportedSrcImage(const int colorFormat, const int width = 1)
{
    return m_srcImageCapacity.flagSupportedColorFormat(colorFormat, width);
}

bool ExynosCameraPP::isSupportedDstImage(const int colorFormat, const int width = 1)
{
    return m_dstImageCapacity.flagSupportedColorFormat(colorFormat, width);
}

status_t ExynosCameraPP::setNextPP(ExynosCameraPP *pp)
{
    if (m_nextPP != NULL) {
        CLOGE("This PP(%s) already have m_nextPP(%s). so, fail", this->getName(), m_nextPP->getName());
        return INVALID_OPERATION;
    }

    m_nextPP = pp;

    return NO_ERROR;
}

ExynosCameraPP *ExynosCameraPP::getNextPP(void)
{
    return m_nextPP;
}

status_t ExynosCameraPP::draw(ExynosCameraImage *srcImage,
                              ExynosCameraImage *dstImage,
                              ExynosCameraParameters *params)
{
    status_t ret = NO_ERROR;

    Mutex::Autolock lock(m_lock);

    int numOfSrc = getNumOfSrcImage();
    int numOfDst = getNumOfDstImage();

    if (this->m_flagCreated() == false) {
        CLOGE("It is not created. so, fail");
        return INVALID_OPERATION;
    }

    ret = ExynosCameraPP::m_draw(srcImage, dstImage, params);
    if (ret != NO_ERROR) {
        CLOGE("ExynosCameraPP::m_draw(numOfSrc(%d), numOfDst(%d)] fail",
                numOfSrc, numOfDst);
        return INVALID_OPERATION;
    }

    ExynosCameraPP *pp = m_getProperPP(this, *srcImage, *dstImage);
    if (pp == NULL) {
        CLOGE("m_getProperPP() fail. so, just try the original this(%s)",
                this->getName());

        pp = this;
    }

    ret = pp->m_draw(srcImage, dstImage, params);
    if (ret != NO_ERROR) {
        char tempStr[EXYNOS_CAMERA_NAME_STR_SIZE];

        snprintf(tempStr, EXYNOS_CAMERA_NAME_STR_SIZE,
                "%s->m_draw(numOfSrc(%d), m_nodeNum(%d)):[SRC] fail",
                pp->getName(), numOfSrc, pp->m_nodeNum);

        pp->m_printImage(tempStr, numOfSrc, srcImage);

        snprintf(tempStr, EXYNOS_CAMERA_NAME_STR_SIZE,
                "%s->m_draw(numOfDst(%d), m_nodeNum(%d)):[DST] fail",
                pp->getName(), numOfDst, pp->m_nodeNum);

        pp->m_printImage(tempStr, numOfDst, dstImage);

        return INVALID_OPERATION;
    }

    return ret;
}

status_t ExynosCameraPP::m_create(void)
{
    status_t ret = NO_ERROR;

    return ret;
}

status_t ExynosCameraPP::m_extControl(__unused int controlType, __unused void *data)
{
    status_t ret = NO_ERROR;

    return ret;
}

status_t ExynosCameraPP::m_destroy(void)
{
    status_t ret = NO_ERROR;

    return ret;
}

bool ExynosCameraPP::m_flagCreated(void)
{
    return m_flagCreate;
}

status_t ExynosCameraPP::m_draw(ExynosCameraImage *srcImage,
                                ExynosCameraImage *dstImage,
                                __unused ExynosCameraParameters *params)
{
    // we can debuging srcImage and dstImage.
    char tempStr[EXYNOS_CAMERA_NAME_STR_SIZE];
    snprintf(tempStr, EXYNOS_CAMERA_NAME_STR_SIZE, "draw():[SRC]");
    m_printImage(tempStr, getNumOfSrcImage(), srcImage);
    snprintf(tempStr, EXYNOS_CAMERA_NAME_STR_SIZE, "draw():[DST]");
    m_printImage(tempStr, getNumOfDstImage(), dstImage);

    return NO_ERROR;
}

ExynosCameraPP *ExynosCameraPP::m_getProperPP(ExynosCameraPP *pp,
                                              ExynosCameraImage srcImage,
                                              ExynosCameraImage dstImage)
{
    ExynosCameraImageCapacity srcImageCapacity = pp->getSrcImageCapacity();
    ExynosCameraImageCapacity dstImageCapacity = pp->getDstImageCapacity();

    if (srcImageCapacity.flagSupportedColorFormat(srcImage.rect.colorFormat, srcImage.rect.fullW) == false ||
        dstImageCapacity.flagSupportedColorFormat(dstImage.rect.colorFormat, dstImage.rect.fullW) == false) {
        if (pp->m_nextPP != NULL) {
            CLOGW("This %s send postprocessing to m_nextPP, to support [SRC]%c%c%c%c, fullW(%d) / [DST]%c%c%c%c, fullW(%d)",
                m_name,
                v4l2Format2Char(srcImage.rect.colorFormat, 0),
                v4l2Format2Char(srcImage.rect.colorFormat, 1),
                v4l2Format2Char(srcImage.rect.colorFormat, 2),
                v4l2Format2Char(srcImage.rect.colorFormat, 3),
                srcImage.rect.fullW,
                v4l2Format2Char(dstImage.rect.colorFormat, 0),
                v4l2Format2Char(dstImage.rect.colorFormat, 1),
                v4l2Format2Char(dstImage.rect.colorFormat, 2),
                v4l2Format2Char(dstImage.rect.colorFormat, 3),
                dstImage.rect.fullW);

            /*
             * call reculsive function.
             */
            return m_getProperPP(pp->m_nextPP, srcImage, dstImage);
        } else {
            char tempStr[EXYNOS_CAMERA_NAME_STR_SIZE];

            if (srcImageCapacity.flagSupportedColorFormat(srcImage.rect.colorFormat, srcImage.rect.fullW) == false) {
                snprintf(tempStr, EXYNOS_CAMERA_NAME_STR_SIZE, "This %s cannot support %c%c%c%c, fullW(%d) (m_nodeNum(%d)):[SRC]",
                    m_name,
                    v4l2Format2Char(srcImage.rect.colorFormat, 0),
                    v4l2Format2Char(srcImage.rect.colorFormat, 1),
                    v4l2Format2Char(srcImage.rect.colorFormat, 2),
                    v4l2Format2Char(srcImage.rect.colorFormat, 3),
                    srcImage.rect.fullW,
                    m_nodeNum);

                m_printImage(tempStr, srcImage, true);
            }

            if (dstImageCapacity.flagSupportedColorFormat(dstImage.rect.colorFormat, dstImage.rect.fullH) == false) {
                snprintf(tempStr, EXYNOS_CAMERA_NAME_STR_SIZE, "This %s cannot support %c%c%c%c, fullW(%d)(m_nodeNum(%d)):[DST]",
                    m_name,
                    v4l2Format2Char(dstImage.rect.colorFormat, 0),
                    v4l2Format2Char(dstImage.rect.colorFormat, 1),
                    v4l2Format2Char(dstImage.rect.colorFormat, 2),
                    v4l2Format2Char(dstImage.rect.colorFormat, 3),
                    dstImage.rect.fullW,
                    m_nodeNum);

                m_printImage(tempStr, dstImage, true);
            }
        }
    } else {
        if (pp->m_flagCreated() == false) {
            status_t ret = NO_ERROR;

            ret = pp->create();
            if (ret != NO_ERROR) {
                CLOGE("pp(%s)->create() fail", pp->getName());
                return NULL;
            }
        }
    }

    return pp;
}

void ExynosCameraPP::m_printImage(char *prefix, ExynosCameraImage image, bool flagLogd)
{
    if (flagLogd == true) {
        CLOGD("%s rect : x(%4d) y(%4d) w(%4d) h(%4d) fullW(%4d) rect.(%4d) colorFormat(%c%c%c%c)",
            prefix,
            image.rect.x,
            image.rect.y,
            image.rect.w,
            image.rect.h,
            image.rect.fullW,
            image.rect.fullH,
            v4l2Format2Char(image.rect.colorFormat, 0),
            v4l2Format2Char(image.rect.colorFormat, 1),
            v4l2Format2Char(image.rect.colorFormat, 2),
            v4l2Format2Char(image.rect.colorFormat, 3));

        CLOGD("%s buf  : index(%2d) planeCount(%2d) : fd[0](%3d) fd[1](%3d) fd[2](%3d) : "
                                                     "addr[0](%p) addr[1](%p) addr[2](%p) : "
                                                     "size[0](%d) size[1](%d) size[2](%d)",
            prefix,
            image.buf.index,
            image.buf.planeCount,
            image.buf.fd[0],
            image.buf.fd[1],
            image.buf.fd[2],
            image.buf.addr[0],
            image.buf.addr[1],
            image.buf.addr[2],
            image.buf.size[0],
            image.buf.size[1],
            image.buf.size[2]);

        CLOGD("%s info : rotation(%3d) flipH(%1d) flipV(%1d)",
            prefix,
            image.rotation,
            image.flipH,
            image.flipV);
    } else {
        CLOGV("%s rect : x(%4d) y(%4d) w(%4d) h(%4d) fullW(%4d) rect.(%4d) colorFormat(%c%c%c%c)",
            prefix,
            image.rect.x,
            image.rect.y,
            image.rect.w,
            image.rect.h,
            image.rect.fullW,
            image.rect.fullH,
            v4l2Format2Char(image.rect.colorFormat, 0),
            v4l2Format2Char(image.rect.colorFormat, 1),
            v4l2Format2Char(image.rect.colorFormat, 2),
            v4l2Format2Char(image.rect.colorFormat, 3));

        CLOGV("%s buf  : index(%2d) planeCount(%2d) : fd[0](%3d) fd[1](%3d) fd[2](%3d) : "
                                                     "addr[0](%p) addr[1](%p) addr[2](%p) : "
                                                     "size[0](%d) size[1](%d) size[2](%d)",
            prefix,
            image.buf.index,
            image.buf.planeCount,
            image.buf.fd[0],
            image.buf.fd[1],
            image.buf.fd[2],
            image.buf.addr[0],
            image.buf.addr[1],
            image.buf.addr[2],
            image.buf.size[0],
            image.buf.size[1],
            image.buf.size[2]);

        CLOGV("%s info : rotation(%3d) flipH(%1d) flipV(%1d)",
            prefix,
            image.rotation,
            image.flipH,
            image.flipV);
    }
}

void ExynosCameraPP::m_printImage(char *prefix, int numOfImage, ExynosCameraImage *image, bool flagLogd)
{
    for (int i = 0; i < numOfImage; i++) {
        if (flagLogd == true) {
            CLOGD("%s [%d] / [%d] rect : ", prefix, i, numOfImage);
            m_printImage(prefix, image[i], flagLogd);
        } else {
            CLOGV("%s [%d] / [%d] rect : ", prefix, i, numOfImage);
            // skip
            // m_printImage(prefix, ExynosCameraImage image[i], flagLogd)
        }
    }
}

void ExynosCameraPP::m_init(void)
{
    m_flagCreate = false;

    m_cameraId = -1;
    memset(m_name, 0x00, sizeof(m_name));
    m_nodeNum  = -1;

    m_srcImageCapacity.setNumOfImage(1);
    m_dstImageCapacity.setNumOfImage(1);

    m_nextPP = NULL;

    m_flagStarted = false;
}

status_t ExynosCameraPP::start(void)
{
    status_t ret = NO_ERROR;

    return ret;
}

status_t ExynosCameraPP::stop(__unused bool suspendFlag)
{
    status_t ret = NO_ERROR;

    return ret;
}

void ExynosCameraPP::setFlagStarted(bool flag)
{
    m_flagStarted = flag;
}

bool ExynosCameraPP::getFlagStarted(void)
{
    return m_flagStarted;
}

