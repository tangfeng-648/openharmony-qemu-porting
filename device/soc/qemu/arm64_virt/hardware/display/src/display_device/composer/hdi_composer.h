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

#ifndef HDI_COMPOSER_H
#define HDI_COMPOSER_H
#include <vector>
#include <memory>
#include "hdi_layer.h"
#include "hdi_drm_layer.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
class HdiComposition {
public:
    HdiComposition() {}
    virtual int32_t Init()
    {
        return DISPLAY_SUCCESS;
    }
    virtual int32_t SetLayers(std::vector<HdiLayer *> &layers, HdiLayer &clientLayer)
    {
        return DISPLAY_SUCCESS;
    }

    #ifdef PLANE_CURSOR_SUPPORT
    virtual int32_t SetCursorLayer(HdiLayer &clientLayer)
    {
        return DISPLAY_SUCCESS;
    }
    #endif

    virtual int32_t Apply(bool modeSet)
    {
        return DISPLAY_SUCCESS;
    }
    virtual ~HdiComposition() {}

protected:
    std::vector<HdiLayer *> mCompLayers;
    DrmGemBuffer * mLastGemBuffer = nullptr;
    DrmGemBuffer * mCurrentGemBuffer = nullptr;


    #ifdef PLANE_CURSOR_SUPPORT
    std::vector<HdiLayer *> mCursorLayers;

    DrmGemBuffer * mLastCursorGemBuffer = nullptr;
    DrmGemBuffer * mCurrentCursorGemBuffer = nullptr;   
    #endif
};

class HdiComposer {
public:
    HdiComposer(std::unique_ptr<HdiComposition> pre, std::unique_ptr<HdiComposition> post);
    virtual ~HdiComposer() {}
    int32_t Prepare(std::vector<HdiLayer *> &layers, HdiLayer &clientLayer);
    int32_t Commit(bool modeSet);
    HdiComposition *GetPreCompostion(uint32_t index)
    {
        if (index >= preComp_.size()) {
            DISPLAY_LOGE("the index is overflow index %{public}d", index);
            return nullptr;
        }
        return preComp_[index].get();
    }

    HdiComposition *GetPostCompostion(uint32_t index)
    {
        if (index >= postComp_.size()) {
            DISPLAY_LOGE("the index is overflow index %{public}d", index);
            return nullptr;
        }
        return postComp_[index].get();
    }

    void AddPostComp(std::unique_ptr<HdiComposition> post)
    {
        postComp_.emplace_back(std::move(post));
    }

    #ifdef PLANE_CURSOR_SUPPORT
    int32_t setCursorLayer(HdiLayer &cursorLayer);
    #endif

private:
    std::vector<std::unique_ptr<HdiComposition>> preComp_;
    std::vector<std::unique_ptr<HdiComposition>> postComp_;
};
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY

#endif // HDI_COMPOSER_H