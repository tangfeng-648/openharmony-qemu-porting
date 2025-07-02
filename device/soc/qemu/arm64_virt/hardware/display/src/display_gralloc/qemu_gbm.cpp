/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 2023.09.14   Phytium     change.
 */

#include "qemu_gbm.h"
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include <securec.h>
#include "qemu_display_log.h"
#include "qemu_gbm_internal.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
using PlaneLayoutInfo = struct {
    uint32_t numPlanes;
    uint32_t radio[MAX_PLANES];
};

using FormatInfo = struct {
    uint32_t format;
    uint32_t bitsPerPixel; // bits per pixel for first plane
    const PlaneLayoutInfo *planes;
};

static const PlaneLayoutInfo g_yuv420SPLayout = {
    .numPlanes = 2,
    .radio = { 4, 2 },
};

static const PlaneLayoutInfo g_yuv420PLayout = {
    .numPlanes = 3,
    .radio = { 4, 1, 1 },
};

static const PlaneLayoutInfo g_yuv422SPLayout = {
    .numPlanes = 2,
    .radio = { 4, 4 },
};

static const PlaneLayoutInfo g_yuv422PLayout = {
    .numPlanes = 3,
    .radio = { 4, 2, 2 },
};

static const FormatInfo *GetFormatInfo(uint32_t format)
{
    static const FormatInfo fmtInfos[] = {
        {DRM_FORMAT_RGBX8888,  32, nullptr},  {DRM_FORMAT_RGBA8888, 32,  nullptr},
        {DRM_FORMAT_BGRX8888,  32, nullptr},  {DRM_FORMAT_BGRA8888, 32,  nullptr},
        {DRM_FORMAT_RGB888,    24, nullptr},  {DRM_FORMAT_RGB565,   16,  nullptr},
        {DRM_FORMAT_BGRX4444,  16, nullptr},  {DRM_FORMAT_BGRA4444, 16,  nullptr},
        {DRM_FORMAT_RGBA4444,  16, nullptr},  {DRM_FORMAT_RGBX4444, 16,  nullptr},
        {DRM_FORMAT_BGRX5551,  16, nullptr},  {DRM_FORMAT_BGRA5551, 16,  nullptr},
        {DRM_FORMAT_NV12, 8, &g_yuv420SPLayout}, {DRM_FORMAT_NV21, 8, &g_yuv420SPLayout},
        {DRM_FORMAT_NV16, 8, &g_yuv422SPLayout},  {DRM_FORMAT_NV61, 8, &g_yuv422SPLayout},
        {DRM_FORMAT_YUV420, 8, &g_yuv420PLayout}, {DRM_FORMAT_YVU420, 8, &g_yuv420PLayout},
        {DRM_FORMAT_YUV422, 8, &g_yuv422PLayout}, {DRM_FORMAT_YVU422, 8, &g_yuv422PLayout},
    };

    for (uint32_t i = 0; i < sizeof(fmtInfos) / sizeof(FormatInfo); i++) {
        if (fmtInfos[i].format == format) {
            return &fmtInfos[i];
        }
    }
    DISPLAY_LOGE("the format can not support");
    return nullptr;
}

void InitGbmBo(struct gbm_bo *bo, const struct drm_mode_create_dumb *dumb)
{
    DISPLAY_CHK_RETURN_NOT_VALUE((dumb == nullptr), DISPLAY_LOGE("dumb is null"));
    DISPLAY_CHK_RETURN_NOT_VALUE((bo == nullptr), DISPLAY_LOGE("bo is null"));
    bo->stride = dumb->pitch;
    bo->size = dumb->size;
    bo->handle = dumb->handle;
}

static uint32_t AlignUp(uint32_t x, uint32_t a)
{
    return ((((x) + ((a)-1)) / (a)) * (a));
}

static uint32_t DivRoundUp(uint32_t n, uint32_t d)
{
    return (((n) + (d)-1) / (d));
}

static uint32_t AdjustStrideFromFormat(uint32_t format, uint32_t height)
{
    const FormatInfo *fmtInfo = GetFormatInfo(format);
    uint32_t adjustheight = height;
    if ((fmtInfo != nullptr) && (fmtInfo->planes != nullptr)) {
        uint32_t sum = fmtInfo->planes->radio[0];
        for (uint32_t i = 1; (i < fmtInfo->planes->numPlanes) && (i < MAX_PLANES); i++) {
            sum += fmtInfo->planes->radio[i];
        }
        if (sum > 0) {
            adjustheight = DivRoundUp((height * sum), fmtInfo->planes->radio[0]);
        }
        DISPLAY_LOGD("height adjust to : %{public}d", adjustheight);
    }
    return adjustheight;
}

struct gbm_bo *hdi_gbm_bo_create(struct gbm_device *gbm, uint32_t width, uint32_t height,
    uint32_t format, uint32_t usage)
{
    int ret;
    struct gbm_bo *bo = nullptr;
    struct drm_mode_create_dumb dumb = { 0 };
    const FormatInfo *fmtInfo = GetFormatInfo(format);
    DISPLAY_CHK_RETURN((fmtInfo == nullptr), NULL, DISPLAY_LOGE("formt: 0x%{public}x can not get layout info", format));
    bo = (struct gbm_bo *)calloc(1, sizeof(struct gbm_bo));
    DISPLAY_CHK_RETURN((bo == nullptr), nullptr, DISPLAY_LOGE("gbm bo create fialed no memery %{public}d", usage));
    (void)memset_s(bo, sizeof(struct gbm_bo), 0, sizeof(struct gbm_bo));
    bo->width = width;
    bo->height = height;
    bo->gbm = gbm;
    bo->format = format;
    // init create_dumb
    dumb.height = AlignUp(AdjustStrideFromFormat(format, height), HEIGHT_ALIGN);
    dumb.width = AlignUp(width, WIDTH_ALIGN);
    dumb.flags = 0;
    dumb.bpp = fmtInfo->bitsPerPixel;
    ret = drmIoctl(gbm->fd, DRM_IOCTL_MODE_CREATE_DUMB, &dumb);
    DISPLAY_LOGI("fmt 0x%{public}x create dumb width: %{public}d  height: %{public}d bpp: %{public}u pitch %{public}d "
        "size %{public}llu",
        format, dumb.width, dumb.height, dumb.bpp, dumb.pitch, dumb.size);
    DISPLAY_CHK_RETURN((ret != 0), nullptr, DISPLAY_LOGE("DRM_IOCTL_MODE_CREATE_DUMB failed errno %{public}d", errno));
    InitGbmBo(bo, &dumb);
    DISPLAY_LOGI(
        "fmt 0x%{public}x create dumb width: %{public}d  height: %{public}d  stride %{public}d size %{public}u", format,
        bo->width, bo->height, bo->stride, bo->size);
    return bo;
}

struct gbm_device *hdi_gbm_create_device(int fd)
{
    struct gbm_device *gbm;
    gbm = (struct gbm_device *)calloc(1, sizeof(struct gbm_device));
    DISPLAY_CHK_RETURN((gbm == nullptr), nullptr, DISPLAY_LOGE("memory calloc failed"));
    gbm->fd = fd;
    return gbm;
}

void hdi_gbm_device_destroy(struct gbm_device *gbm)
{
    free(gbm);
}

uint32_t hdi_gbm_bo_get_stride(struct gbm_bo *bo)
{
    DISPLAY_CHK_RETURN((bo == nullptr), 0, DISPLAY_LOGE("the bo is null"));
    return bo->stride;
}

uint32_t hdi_gbm_bo_get_width(struct gbm_bo *bo)
{
    DISPLAY_CHK_RETURN((bo == nullptr), 0, DISPLAY_LOGE("the bo is null"));
    return bo->width;
}

uint32_t hdi_gbm_bo_get_height(struct gbm_bo *bo)
{
    DISPLAY_CHK_RETURN((bo == nullptr), 0, DISPLAY_LOGE("the bo is null"));
    return bo->height;
}

uint32_t hdi_gbm_bo_get_size(struct gbm_bo *bo)
{
    DISPLAY_CHK_RETURN((bo == nullptr), 0, DISPLAY_LOGE("the bo is null"));
    return bo->size;
}

void hdi_gbm_bo_destroy(struct gbm_bo *bo)
{
    int ret;
    DISPLAY_CHK_RETURN_NOT_VALUE((bo == nullptr), DISPLAY_LOGE("the bo is null"));
    struct drm_mode_destroy_dumb dumb = { 0 };
    dumb.handle = bo->handle;
    ret = drmIoctl(bo->gbm->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dumb);
    DISPLAY_CHK_RETURN_NOT_VALUE((ret), DISPLAY_LOGE("dumb buffer destroy failed errno %{public}d", errno));
    free(bo);
}

int hdi_gbm_bo_get_fd(struct gbm_bo *bo)
{
    int fd;
	int ret;
    ret = drmPrimeHandleToFD(bo->gbm->fd, bo->handle, DRM_CLOEXEC | DRM_RDWR, &fd);
    DISPLAY_CHK_RETURN((ret), -1, \
        DISPLAY_LOGE("drmPrimeHandleToFD  failed ret: %{public}d  errno: %{public}d", ret, errno));
    return fd;
}
} // namespace DISPLAY
} // namespace HDI
} // namespace OHOS
