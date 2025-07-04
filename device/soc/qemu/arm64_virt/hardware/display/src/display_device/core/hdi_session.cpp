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
 */

#include <hdi_session.h>
#include <cerrno>
#include <mutex>
#include "common/include/display_common.h"
#include "qemu_display_log.h"
#include "v1_0/display_composer_type.h"
#include "hdf_trace.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
HdiSession &HdiSession::GetInstance()
{
    static HdiSession instance;
    static std::once_flag once;
    std::call_once(once, [&]() { instance.Init(); });
    return instance;
}

void HdiSession::Init()
{
    DISPLAY_LOGD();
    mHdiDevices = HdiDeviceInterface::DiscoveryDevice();
    DISPLAY_LOGD("devices size %{public}zd", mHdiDevices.size());
    mHdiDisplays.clear();
    for (auto device : mHdiDevices) {
        auto displays = device->DiscoveryDisplay();
        mHdiDisplays.insert(displays.begin(), displays.end());
    }
}

int32_t HdiSession::RegHotPlugCallback(HotPlugCallback callback, void *data)
{
    DISPLAY_CHK_RETURN((callback == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("the callback is nullptr"));
    mHotPlugCallBacks.emplace(callback, data);
    for (auto displayMap : mHdiDisplays) {
        auto display = displayMap.second;
        if (display->IsConnected()) {
            DoHotPlugCallback(display->GetId(), true);
        }
    }
    return DISPLAY_SUCCESS;
}

void HdiSession::DoHotPlugCallback(uint32_t devId, bool connect)
{
    DISPLAY_LOGD();
    for (const auto &callback : mHotPlugCallBacks) {
        callback.first(devId, connect, callback.second);
    }
}
} // OHOS
} // HDI
} // DISPLAY

using namespace OHOS::HDI::DISPLAY;
static int32_t RegHotPlugCallback(HotPlugCallback callback, void *data)
{
    DISPLAY_LOGD();
    HdiSession::GetInstance().RegHotPlugCallback(callback, data);
    return DISPLAY_SUCCESS;
}

static int32_t GetDisplayCapability(uint32_t devId, DisplayCapability *info)
{
    DISPLAY_LOGD();
    DISPLAY_CHK_RETURN(info == nullptr, DISPLAY_NULL_PTR, DISPLAY_LOGE("info is nullptr"));
    return HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayCapability, info);
}

static int32_t GetDisplaySupportedModes(uint32_t devId, uint32_t *num, DisplayModeInfo *modes)
{
    DISPLAY_LOGD();
    DISPLAY_CHK_RETURN(num == nullptr, DISPLAY_NULL_PTR, DISPLAY_LOGE("num is nullptr"));
    return HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplaySupportedModes, num, modes);
}

static int32_t GetDisplayMode(uint32_t devId, uint32_t *mode)
{
    DISPLAY_LOGD();
    DISPLAY_CHK_RETURN((mode == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("mode is nullptr"));
    return HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayMode, mode);
}

static int32_t SetDisplayMode(uint32_t devId, uint32_t mode)
{
    DISPLAY_LOGD();
    return HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::SetDisplayMode, mode);
}

static int32_t GetDisplayPowerStatus(uint32_t devId, DispPowerStatus *status)
{
    DISPLAY_LOGD();
    DISPLAY_CHK_RETURN((status == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("status is nullptr"));
    return HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayPowerStatus, status);
}

static int32_t SetDisplayPowerStatus(uint32_t devId, DispPowerStatus status)
{
    DISPLAY_LOGD();
    return HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::SetDisplayPowerStatus, status);
}

static int32_t GetDisplayBacklight(uint32_t devId, uint32_t *value)
{
    DISPLAY_LOGD();
    DISPLAY_CHK_RETURN((value == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("value is nullptr"));
    return HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayBacklight, value);
}

static int32_t SetDisplayBacklight(uint32_t devId, uint32_t value)
{
    DISPLAY_LOGD();
    return HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::SetDisplayBacklight, value);
}

static int32_t GetDisplayProperty(uint32_t devId, uint32_t id, uint64_t *value)
{
    DISPLAY_LOGD();
    (void)id;
    DISPLAY_CHK_RETURN((value == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("value is nullptr"));
    return DISPLAY_NOT_SUPPORT;
}

static int32_t SetDisplayProperty(uint32_t devId, uint32_t id, uint64_t value)
{
    DISPLAY_LOGD();
    (void)id;
    return DISPLAY_NOT_SUPPORT;
}

static int32_t PrepareDisplayLayers(uint32_t devId, bool *needFlushFb)
{
    DISPLAY_LOGD();
    DISPLAY_CHK_RETURN((needFlushFb == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("needFlushFb is nullptr"));
    return HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::PrepareDisplayLayers, needFlushFb);
}

static int32_t GetDisplayCompChange(uint32_t devId, uint32_t *num, uint32_t *layers, int32_t *type)
{
    DISPLAY_LOGD();
    DISPLAY_CHK_RETURN((num == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("num is nullptr"));
    return HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayCompChange, num, layers, type);
}

static int32_t SetDisplayClientCrop(uint32_t devId, IRect *rect)
{
    DISPLAY_LOGD();
    DISPLAY_CHK_RETURN((rect == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("rect is nullptr"));
    return DISPLAY_NOT_SUPPORT;
}

static int32_t SetDisplayClientDestRect(uint32_t devId, IRect *rect)
{
    DISPLAY_LOGD();
    DISPLAY_CHK_RETURN((rect == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("rect is nullptr"));
    return DISPLAY_NOT_SUPPORT;
}

static int32_t SetDisplayClientBuffer(uint32_t devId, const BufferHandle *buffer, int32_t fence)
{
    DISPLAY_LOGD();
    return HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::SetDisplayClientBuffer, buffer, fence);
}

static int32_t SetDisplayClientDamage(uint32_t devId, uint32_t num, IRect *rect)
{
    DISPLAY_LOGD();
    (void)num;
    DISPLAY_CHK_RETURN((rect == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("rect is nullptr"));
    return DISPLAY_NOT_SUPPORT;
}

static int32_t SetDisplayVsyncEnabled(uint32_t devId, bool enabled)
{
    DISPLAY_LOGD();
    return HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::SetDisplayVsyncEnabled, enabled);
}

static int32_t RegDisplayVBlankCallback(uint32_t devId, VBlankCallback callback, void *data)
{
    DISPLAY_LOGD();
    return HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::RegDisplayVBlankCallback, callback, data);
}

static int32_t GetDisplayReleaseFence(uint32_t devId, uint32_t *num, uint32_t *layers, int32_t *fences)
{
    DISPLAY_LOGD();
    return HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayReleaseFence, num, layers,
        fences);
}

static int32_t Commit(uint32_t devId, int32_t *fence)
{
    DISPLAY_LOGD();
    DISPLAY_CHK_RETURN((fence == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("fence is nullptr"));
    return HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::Commit, fence);
}

static int32_t CreateVirtualDisplay(uint32_t width, uint32_t height, int32_t *format, uint32_t *devId)
{
    DISPLAY_LOGD();
    return DISPLAY_NOT_SUPPORT;
}
static int32_t DestroyVirtualDisplay(uint32_t devId)
{
    DISPLAY_LOGD();
    return DISPLAY_NOT_SUPPORT;
}
static int32_t SetVirtualDisplayBuffer(uint32_t devId, BufferHandle *buffer, int32_t releaseFence)
{
    DISPLAY_LOGD();
    return DISPLAY_NOT_SUPPORT;
}


// Layer function
static int32_t CreateLayer(uint32_t devId, const LayerInfo *layerInfo, uint32_t *layerId)
{
    DISPLAY_LOGD();
    DISPLAY_CHK_RETURN((layerId == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("layerId is nullptr"));
    return HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::CreateLayer, layerInfo, layerId);
}

static int32_t DestroyLayer(uint32_t devId, uint32_t layerId)
{
    DISPLAY_LOGD();
    return HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::DestroyLayer, layerId);
}

static int32_t SetLayerRegion(uint32_t devId, uint32_t layerId, IRect *rect)
{
    DISPLAY_CHK_RETURN((rect == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("rect is nullptr"));
    DISPLAY_LOGD();
    return HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerRegion, rect);
}

static int32_t SetLayerCrop(uint32_t devId, uint32_t layerId, IRect *rect)
{
    DISPLAY_LOGD();
    DISPLAY_CHK_RETURN((rect == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("rect is nullptr"));
    return HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerCrop, rect);
}

static int32_t SetLayerZorder(uint32_t devId, uint32_t layerId, uint32_t zorder)
{
    DISPLAY_LOGD();
    return HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::SetLayerZorder, layerId, zorder);
}

static int32_t SetLayerPreMulti(uint32_t devId, uint32_t layerId, bool preMul)
{
    DISPLAY_LOGD();
    return HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerPreMulti, preMul);
}

static int32_t SetLayerAlpha(uint32_t devId, uint32_t layerId, LayerAlpha *alpha)
{
    DISPLAY_LOGD();
    DISPLAY_CHK_RETURN((alpha == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("alpha is nullptr"));
    return HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerAlpha, alpha);
}

static int32_t SetLayerTransformMode(uint32_t devId, uint32_t layerId, TransformType type)
{
    DISPLAY_LOGD();
    return HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerTransformMode, type);
}

static int32_t SetLayerDirtyRegion(uint32_t devId, uint32_t layerId, IRect *region)
{
    DISPLAY_LOGD();
    DISPLAY_CHK_RETURN((region == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("region is nullptr"));
    return HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerDirtyRegion, region);
}

static int32_t SetLayerVisibleRegion(uint32_t devId, uint32_t layerId, uint32_t num, IRect *rect)
{
    DISPLAY_LOGD();
    DISPLAY_CHK_RETURN((rect == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("rect is nullptr"));
    return HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerVisibleRegion, num, rect);
}

static int32_t SetLayerBuffer(uint32_t devId, uint32_t layerId, const BufferHandle *buffer, int32_t fence)
{
    DISPLAY_LOGD();
    return HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerBuffer, buffer, fence);
}

static int32_t SetLayerCompositionType(uint32_t devId, uint32_t layerId, CompositionType type)
{
    DISPLAY_LOGD();
    return HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerCompositionType, type);
}

static int32_t SetLayerBlendType(uint32_t devId, uint32_t layerId, BlendType type)
{
    DISPLAY_LOGD();
    return HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerBlendType, type);
}

