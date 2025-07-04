/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
 * 2023.09.14   Phytium     change log info.
 */

#include "hdi_gfx_composition.h"
#include <cinttypes>
#include <dlfcn.h>
#include <cerrno>
#include "qemu_display_log.h"
#include "display_gfx.h"
#include "v1_0/display_composer_type.h"

using namespace OHOS::HDI::Display::Composer::V1_0;

namespace OHOS {
namespace HDI {
namespace DISPLAY {
int32_t HdiGfxComposition::Init(void)
{
    DISPLAY_LOGD();
    int32_t ret = GfxModuleInit();
    if ((ret != DISPLAY_SUCCESS) || (mGfxFuncs == nullptr)) {
        DISPLAY_LOGE("GfxModuleInit failed will use client composition always");
        return DISPLAY_SUCCESS;
    }
    ret = mGfxFuncs->InitGfx();
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("gfx init failed"));
    if (ret != DISPLAY_SUCCESS) {
        DISPLAY_LOGE("Failed to init gfx will use client composition always");
        return DISPLAY_SUCCESS;
    }
    valid_ = true;
    return DISPLAY_SUCCESS;
}

int32_t HdiGfxComposition::GfxModuleInit(void)
{
    DISPLAY_LOGD();
    mGfxModule = dlopen(LIB_HDI_GFX_NAME, RTLD_NOW | RTLD_NOLOAD);
    if (mGfxModule != nullptr) {
        DISPLAY_LOGI("Module '%{public}s' already loaded", LIB_HDI_GFX_NAME);
    } else {
        DISPLAY_LOGI("Loading module '%{public}s'", LIB_HDI_GFX_NAME);
        mGfxModule = dlopen(LIB_HDI_GFX_NAME, RTLD_NOW);
        if (mGfxModule == nullptr) {
            DISPLAY_LOGE("Failed to load module: %{public}s", dlerror());
            return DISPLAY_FAILURE;
        }
    }

    using InitFunc = int32_t (*)(GfxFuncs **funcs);
    InitFunc func = (int32_t (*)(GfxFuncs **funcs))(dlsym(mGfxModule, LIB_GFX_FUNC_INIT));
    if (func == nullptr) {
        DISPLAY_LOGE("Failed to lookup %{public}s function: %s", LIB_GFX_FUNC_INIT, dlerror());
        dlclose(mGfxModule);
        return DISPLAY_FAILURE;
    }
    return func(&mGfxFuncs);
}

int32_t HdiGfxComposition::GfxModuleDeinit(void)
{
    DISPLAY_LOGD();
    int32_t ret = DISPLAY_SUCCESS;
    if (mGfxModule == nullptr) {
        using DeinitFunc = int32_t (*)(GfxFuncs *funcs);
        DeinitFunc func = (int32_t (*)(GfxFuncs *funcs))(dlsym(mGfxModule, LIB_GFX_FUNC_DEINIT));
        if (func == nullptr) {
            DISPLAY_LOGE("Failed to lookup %{public}s function: %s", LIB_GFX_FUNC_DEINIT, dlerror());
        } else {
            ret = func(mGfxFuncs);
        }
        dlclose(mGfxModule);
    }
    return ret;
}

bool HdiGfxComposition::CanHandle(HdiLayer &hdiLayer)
{
    DISPLAY_LOGD();
    (void)hdiLayer;
    return true;
}

int32_t HdiGfxComposition::SetLayers(std::vector<HdiLayer *> &layers, HdiLayer &clientLayer)
{
    DISPLAY_LOGD("layers size %{public}zd", layers.size());
    mClientLayer = &clientLayer;
    mCompLayers.clear();
    for (auto &layer : layers) {
        if (CanHandle(*layer)) {
            if ((layer->GetCompositionType() != COMPOSITION_VIDEO) &&
                (layer->GetCompositionType() != COMPOSITION_CURSOR)) {
                layer->SetDeviceSelect(COMPOSITION_DEVICE);
            } else {
                layer->SetDeviceSelect(layer->GetCompositionType());
            }
            mCompLayers.push_back(layer);
        } else {
            layer->SetDeviceSelect(COMPOSITION_CLIENT);
        }
    }
    DISPLAY_LOGD("composer layers size %{public}zd", mCompLayers.size());
    return DISPLAY_SUCCESS;
}

void HdiGfxComposition::InitGfxSurface(ISurface &surface, HdiLayerBuffer &buffer)
{
    surface.width = buffer.GetWidth();
    surface.height = buffer.GetHeight();
    surface.phyAddr = buffer.GetMemHandle();
    surface.enColorFmt = (PixelFormat)buffer.GetFormat();
    surface.stride = buffer.GetStride();
    surface.bAlphaExt1555 = true;
    surface.bAlphaMax255 = true;
    surface.alpha0 = 0XFF;
    surface.alpha1 = 0XFF;
    DISPLAY_LOGD("surface w:%{public}d h:%{public}d fmt:%{public}d stride:%{public}d",
        surface.width, surface.height, surface.enColorFmt, surface.stride);
}

// now not handle the alpha of layer
int32_t HdiGfxComposition::BlitLayer(HdiLayer &src, HdiLayer &dst)
{
    ISurface srcSurface = { 0 };
    ISurface dstSurface = { 0 };
    GfxOpt opt = { 0 };
    src.WaitAcquireFence();
    DISPLAY_LOGD();
    HdiLayerBuffer *srcBuffer = src.GetCurrentBuffer();
    DISPLAY_CHK_RETURN((srcBuffer == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("the srcbuffer is null"));
    DISPLAY_LOGD("init the src surface");
    InitGfxSurface(srcSurface, *srcBuffer);

    HdiLayerBuffer *dstBuffer = dst.GetCurrentBuffer();
    DISPLAY_CHK_RETURN((dstBuffer == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("can not get client layer buffer"));
    DISPLAY_LOGD("init the dst surface");
    InitGfxSurface(dstSurface, *dstBuffer);

    opt.blendType = src.GetLayerBlenType();
    DISPLAY_LOGD("blendType %{public}d", opt.blendType);
    opt.enPixelAlpha = true;
    opt.enableScale = true;

    if (src.GetAlpha().enGlobalAlpha) { // is alpha is 0xff we not set it
        opt.enGlobalAlpha = true;
        srcSurface.alpha0 = src.GetAlpha().gAlpha;
        DISPLAY_LOGD("src alpha %{public}x", src.GetAlpha().gAlpha);
    }
    opt.rotateType = src.GetTransFormType();
    DISPLAY_LOGD(" the roate type is %{public}d", opt.rotateType);
    IRect crop = src.GetLayerCrop();
    IRect displayRect = src.GetLayerDisplayRect();
    DISPLAY_LOGD("crop x: %{public}d y : %{public}d w : %{public}d h: %{public}d", crop.x, crop.y, crop.w, crop.h);
    DISPLAY_LOGD("displayRect x: %{public}d y : %{public}d w : %{public}d h : %{public}d", displayRect.x, displayRect.y,
        displayRect.w, displayRect.h);
    DISPLAY_CHK_RETURN(mGfxFuncs == nullptr, DISPLAY_FAILURE, DISPLAY_LOGE("Blit: mGfxFuncs is null"));
    return mGfxFuncs->Blit(&srcSurface, &crop, &dstSurface, &displayRect, &opt);
}

int32_t HdiGfxComposition::ClearRect(HdiLayer &src, HdiLayer &dst)
{
    ISurface dstSurface = { 0 };
    GfxOpt opt = { 0 };
    DISPLAY_LOGD();
    HdiLayerBuffer *dstBuffer = dst.GetCurrentBuffer();
    DISPLAY_CHK_RETURN((dstBuffer == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("can not get client layer buffer"));
    InitGfxSurface(dstSurface, *dstBuffer);
    IRect rect = src.GetLayerDisplayRect();
    DISPLAY_CHK_RETURN(mGfxFuncs == nullptr, DISPLAY_FAILURE, DISPLAY_LOGE("Rect: mGfxFuncs is null"));
    return mGfxFuncs->FillRect(&dstSurface, &rect, 0, &opt);
}

int32_t HdiGfxComposition::Apply(bool modeSet)
{
    int32_t ret;
    DISPLAY_LOGD("composer layers size %{public}zd", mCompLayers.size());
    for (uint32_t i = 0; i < mCompLayers.size(); i++) {
        HdiLayer *layer = mCompLayers[i];
        CompositionType compType = layer->GetDeviceSelect();
        switch (compType) {
            case COMPOSITION_VIDEO:
                ret = ClearRect(*layer, *mClientLayer);
                DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE,
                    DISPLAY_LOGE("clear layer %{public}d failed", i));
                break;
            case COMPOSITION_DEVICE:
                ret = BlitLayer(*layer, *mClientLayer);
                DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE,
                    DISPLAY_LOGE("blit layer %{public}d failed ", i));
                break;
            case COMPOSITION_CLIENT:
                break;
            default:
                DISPLAY_LOGE("the gfx composition can not surpport the type %{public}d", compType);
                break;
        }
    }
    return DISPLAY_SUCCESS;
}
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY
