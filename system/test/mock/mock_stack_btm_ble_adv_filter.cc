/*
 * Copyright 2021 The Android Open Source Project
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

/*
 * Generated mock file from original source file
 *   Functions generated:6
 */

#include <base/functional/bind.h>
#include <string.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "bind_helpers.h"
#include "bt_target.h"
#include "btm_ble_api.h"
#include "btu.h"
#include "device/include/controller.h"
#include "stack/btm/btm_ble_int.h"
#include "stack/btm/btm_int_types.h"
#include "test/common/mock_functions.h"

#ifndef UNUSED_ATTR
#define UNUSED_ATTR
#endif

void BTM_BleAdvFilterParamSetup(
    tBTM_BLE_SCAN_COND_OP action, tBTM_BLE_PF_FILT_INDEX filt_index,
    std::unique_ptr<btgatt_filt_param_setup_t> p_filt_params,
    tBTM_BLE_PF_PARAM_CB cb) {
  inc_func_call_count(__func__);
}
void BTM_BleEnableDisableFilterFeature(uint8_t enable,
                                       tBTM_BLE_PF_STATUS_CBACK p_stat_cback) {
  inc_func_call_count(__func__);
}
void BTM_LE_PF_clear(tBTM_BLE_PF_FILT_INDEX filt_index,
                     tBTM_BLE_PF_CFG_CBACK cb) {
  inc_func_call_count(__func__);
}
void BTM_LE_PF_set(tBTM_BLE_PF_FILT_INDEX filt_index,
                   std::vector<ApcfCommand> commands,
                   tBTM_BLE_PF_CFG_CBACK cb) {
  inc_func_call_count(__func__);
}
void btm_ble_adv_filter_init(void) { inc_func_call_count(__func__); }
