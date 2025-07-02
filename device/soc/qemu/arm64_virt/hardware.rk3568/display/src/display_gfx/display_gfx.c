/*
 * Copyright (c) 2021-2023 Rockchip Electronics Co., Ltd.
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

#include <stdio.h>
#include <string.h>
#include "display_log.h"
#include "securec.h"
#include "display_gfx.h"

int32_t GfxInitialize(GfxFuncs **funcs)
{
    if (funcs == NULL)
        DISPLAY_LOGW("info is null");
    
    DISPLAY_LOGI("%s: gfx initialize success", __func__);
    return DISPLAY_SUCCESS;
}

int32_t GfxUninitialize(GfxFuncs *funcs)
{
    if (funcs == NULL) {
        DISPLAY_LOGW("%s: funcs is null", __func__);
        return DISPLAY_SUCCESS;
    }
    free(funcs);
    DISPLAY_LOGD("%s: gfx uninitialize success", __func__);
    return DISPLAY_SUCCESS;
}
