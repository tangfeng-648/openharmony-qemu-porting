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

#include "drm_encoder.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
DrmEncoder::DrmEncoder(drmModeEncoder e)
{
    mEncoderId = e.encoder_id;
    mCrtcId = e.crtc_id;
    mPossibleCrtcs = e.possible_crtcs;
    DISPLAY_LOGD("mEncoderId = %{public}u, mCrtcId = %{public}u, mPossibleCrtcs = %{public}u", mEncoderId, mCrtcId, mPossibleCrtcs);
}

int32_t DrmEncoder::PickIdleCrtcId(IdMapPtr<DrmCrtc> &crtcs, uint32_t &crtcId)
{
    // find the crtc id;
    DISPLAY_LOGD("crtcs size %{public}zu", crtcs.size());
    std::shared_ptr<DrmCrtc> crtc;
    auto crtcIter = crtcs.find(mCrtcId);
    if (crtcIter == crtcs.end()) {
        DISPLAY_LOGW("can not find crtc for id %{public}d", mCrtcId);
        crtcIter = crtcs.begin();
        crtc = nullptr;
    }
    else
        crtc = crtcIter->second;

    if (!crtc) {
        DISPLAY_LOGE("encoder fails to get kernel-state crtc_id, re-selects based on possible crtc_id");
        for (const auto &posCrtcPair : crtcs) {
            auto &posCrts = posCrtcPair.second;
            DISPLAY_LOGD("try crtc id : %{public}d", posCrts->GetId());
            if (posCrts->CanBind() && ((1 << posCrts->GetPipe()) & mPossibleCrtcs)) {
                crtc = posCrts;
            }
        }
    }
    DISPLAY_CHK_RETURN((crtc == nullptr), DISPLAY_FAILURE,
        DISPLAY_LOGE("encoder %{public}d can not bind to idle crtc", mEncoderId));
    crtcId = crtc->GetId();
    return DISPLAY_SUCCESS;
}
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY