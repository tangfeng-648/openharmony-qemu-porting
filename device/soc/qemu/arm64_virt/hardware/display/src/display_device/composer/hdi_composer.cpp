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

#include "hdi_composer.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
HdiComposer::HdiComposer(std::unique_ptr<HdiComposition> pre, std::unique_ptr<HdiComposition> post)
{
    preComp_.emplace_back(std::move(pre));
    postComp_.emplace_back(std::move(post));
}

int32_t HdiComposer::Prepare(std::vector<HdiLayer *> &layers, HdiLayer &clientLayer)
{
    DISPLAY_LOGD();
    int32_t ret;

    // for (auto& composition : preComp_) {
    //     ret = composition->SetLayers(layers, clientLayer);
    //     DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("pre composition prepare failed"));
    // }

    for (auto& composition : postComp_) {
        ret = composition->SetLayers(layers, clientLayer);
        DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("post composition prepare failed"));
    }

    return DISPLAY_SUCCESS;
}

#ifdef PLANE_CURSOR_SUPPORT
int32_t HdiComposer::setCursorLayer(HdiLayer &cursorLayer){
    int ret;
    for (auto& composition : postComp_) {
        ret = composition->SetCursorLayer(cursorLayer);
        DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("setCursorLayer failed"));
    }
    return DISPLAY_SUCCESS;    
}
#endif

int32_t HdiComposer::Commit(bool modeSet)
{
    DISPLAY_LOGD();
    int32_t ret;
    // for (auto& composition : preComp_) {
    //     ret = composition->Apply(modeSet);
    //     DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("pre composition prepare failed"));
    // }

    for (auto& composition : postComp_) {
        ret = composition->Apply(modeSet);
        DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("post composition prepare failed"));
    }
    return DISPLAY_SUCCESS;
}
} // OHOS
} // HDI
} // DISPLAY
