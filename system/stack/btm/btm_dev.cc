/******************************************************************************
 *
 *  Copyright 1999-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  This file contains functions for the Bluetooth Device Manager
 *
 ******************************************************************************/

#include "stack/btm/btm_dev.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "btm_api.h"
#include "btm_ble_int.h"
#include "device/include/controller.h"
#include "l2c_api.h"
#include "main/shim/btm_api.h"
#include "main/shim/dumpsys.h"
#include "main/shim/shim.h"
#include "osi/include/allocator.h"
#include "osi/include/compat.h"
#include "rust/src/connection/ffi/connection_shim.h"
#include "stack/include/acl_api.h"
#include "stack/include/bt_octets.h"
#include "types/raw_address.h"

extern tBTM_CB btm_cb;
void gatt_consolidate(const RawAddress& identity_addr, const RawAddress& rpa);

namespace {

constexpr char kBtmLogTag[] = "BOND";

}

/*******************************************************************************
 *
 * Function         BTM_SecAddDevice
 *
 * Description      Add/modify device.  This function will be normally called
 *                  during host startup to restore all required information
 *                  stored in the NVRAM.
 *
 * Parameters:      bd_addr          - BD address of the peer
 *                  dev_class        - Device Class
 *                  bd_name          - Name of the peer device. NULL if unknown.
 *                  features         - Remote device's features (up to 3 pages).
 *                                     NULL if not known
 *                  link_key         - Connection link key. NULL if unknown.
 *
 * Returns          true if added OK, else false
 *
 ******************************************************************************/
bool BTM_SecAddDevice(const RawAddress& bd_addr, DEV_CLASS dev_class,
                      const BD_NAME& bd_name, uint8_t* features,
                      LinkKey* p_link_key, uint8_t key_type,
                      uint8_t pin_length) {
  tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev(bd_addr);
  if (!p_dev_rec) {
    p_dev_rec = btm_sec_allocate_dev_rec();
    LOG_DEBUG(
        "Caching new record from config file device:%s link_key_type:%x "
        "name:%s",
        ADDRESS_TO_LOGGABLE_CSTR(bd_addr), key_type, bd_name);

    p_dev_rec->bd_addr = bd_addr;
    p_dev_rec->hci_handle = BTM_GetHCIConnHandle(bd_addr, BT_TRANSPORT_BR_EDR);

    /* use default value for background connection params */
    /* update conn params, use default value for background connection params */
    memset(&p_dev_rec->conn_params, 0xff, sizeof(tBTM_LE_CONN_PRAMS));
  } else {
    LOG_DEBUG(
        "Caching existing record from config file device:%s link_key_type:%x",
        ADDRESS_TO_LOGGABLE_CSTR(bd_addr), key_type);

    /* "Bump" timestamp for existing record */
    p_dev_rec->timestamp = btm_cb.dev_rec_count++;

    /* TODO(eisenbach):
     * Small refactor, but leaving original logic for now.
     * On the surface, this does not make any sense at all. Why change the
     * bond state for an existing device here? This logic should be verified
     * as part of a larger refactor.
     */
    p_dev_rec->bond_type = tBTM_SEC_DEV_REC::BOND_TYPE_UNKNOWN;
  }

  if (dev_class) memcpy(p_dev_rec->dev_class, dev_class, DEV_CLASS_LEN);

  memset(p_dev_rec->sec_bd_name, 0, sizeof(tBTM_BD_NAME));

  if (bd_name && bd_name[0]) {
    LOG_DEBUG("  Remote name known for device:%s name:%s",
              ADDRESS_TO_LOGGABLE_CSTR(bd_addr), bd_name);
    p_dev_rec->sec_flags |= BTM_SEC_NAME_KNOWN;
    strlcpy((char*)p_dev_rec->sec_bd_name, (char*)bd_name,
            BTM_MAX_REM_BD_NAME_LEN + 1);
  }

  if (p_link_key) {
    LOG_DEBUG("  Link key known for device:%s", ADDRESS_TO_LOGGABLE_CSTR(bd_addr));
    p_dev_rec->sec_flags |= BTM_SEC_LINK_KEY_KNOWN;
    p_dev_rec->link_key = *p_link_key;
    p_dev_rec->link_key_type = key_type;
    p_dev_rec->pin_code_length = pin_length;

    if (pin_length >= 16 || key_type == BTM_LKEY_TYPE_AUTH_COMB ||
        key_type == BTM_LKEY_TYPE_AUTH_COMB_P_256) {
      // Set the flag if the link key was made by using either a 16 digit
      // pin or MITM.
      p_dev_rec->sec_flags |=
          BTM_SEC_16_DIGIT_PIN_AUTHED | BTM_SEC_LINK_KEY_AUTHED;
    }
  }

  p_dev_rec->rmt_io_caps = BTM_IO_CAP_OUT;
  p_dev_rec->device_type |= BT_DEVICE_TYPE_BREDR;

  return true;
}

void wipe_secrets_and_remove(tBTM_SEC_DEV_REC* p_dev_rec) {
  p_dev_rec->link_key.fill(0);
  memset(&p_dev_rec->ble.keys, 0, sizeof(tBTM_SEC_BLE_KEYS));
  list_remove(btm_cb.sec_dev_rec, p_dev_rec);
}

/** Removes the device from acceptlist */
void BTM_AcceptlistRemove(const RawAddress& address);

/** Free resources associated with the device associated with |bd_addr| address.
 *
 * *** WARNING ***
 * tBTM_SEC_DEV_REC associated with bd_addr becomes invalid after this function
 * is called, also any of it's fields. i.e. if you use p_dev_rec->bd_addr, it is
 * no longer valid!
 * *** WARNING ***
 *
 * Returns true if removed OK, false if not found or ACL link is active.
 */
bool BTM_SecDeleteDevice(const RawAddress& bd_addr) {
  if (BTM_IsAclConnectionUp(bd_addr, BT_TRANSPORT_LE) ||
      BTM_IsAclConnectionUp(bd_addr, BT_TRANSPORT_BR_EDR)) {
    LOG_WARN("%s FAILED: Cannot Delete when connection to %s is active",
             __func__, ADDRESS_TO_LOGGABLE_CSTR(bd_addr));
    return false;
  }

  tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev(bd_addr);
  if (p_dev_rec != NULL) {
    RawAddress bda = p_dev_rec->bd_addr;

    LOG_INFO("Remove device %s from filter accept list before delete record",
             ADDRESS_TO_LOGGABLE_CSTR(bd_addr));
    if (bluetooth::common::init_flags::
            use_unified_connection_manager_is_enabled()) {
      bluetooth::connection::GetConnectionManager()
          .stop_all_connections_to_device(
              bluetooth::connection::ResolveRawAddress(p_dev_rec->bd_addr));
    } else {
      BTM_AcceptlistRemove(p_dev_rec->bd_addr);
    }

    const auto device_type = p_dev_rec->device_type;
    const auto bond_type = p_dev_rec->bond_type;

    /* Clear out any saved BLE keys */
    btm_sec_clear_ble_keys(p_dev_rec);
    wipe_secrets_and_remove(p_dev_rec);
    /* Tell controller to get rid of the link key, if it has one stored */
    BTM_DeleteStoredLinkKey(&bda, NULL);
    LOG_INFO("%s %s complete", __func__, ADDRESS_TO_LOGGABLE_CSTR(bd_addr));
    BTM_LogHistory(kBtmLogTag, bd_addr, "Device removed",
                   base::StringPrintf("device_type:%s bond_type:%s",
                                      DeviceTypeText(device_type).c_str(),
                                      bond_type_text(bond_type).c_str()));
  } else {
    LOG_WARN("%s Unable to delete link key for unknown device %s", __func__,
             ADDRESS_TO_LOGGABLE_CSTR(bd_addr));
  }

  return true;
}

/*******************************************************************************
 *
 * Function         BTM_SecClearSecurityFlags
 *
 * Description      Reset the security flags (mark as not-paired) for a given
 *                  remove device.
 *
 ******************************************************************************/
void BTM_SecClearSecurityFlags(const RawAddress& bd_addr) {
  tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev(bd_addr);
  if (p_dev_rec == NULL) return;

  p_dev_rec->sec_flags = 0;
  p_dev_rec->sec_state = BTM_SEC_STATE_IDLE;
  p_dev_rec->sm4 = BTM_SM4_UNKNOWN;
}

/*******************************************************************************
 *
 * Function         BTM_SecReadDevName
 *
 * Description      Looks for the device name in the security database for the
 *                  specified BD address.
 *
 * Returns          Pointer to the name or NULL
 *
 ******************************************************************************/
char* BTM_SecReadDevName(const RawAddress& bd_addr) {
  char* p_name = NULL;
  tBTM_SEC_DEV_REC* p_srec;

  p_srec = btm_find_dev(bd_addr);
  if (p_srec != NULL) p_name = (char*)p_srec->sec_bd_name;

  return (p_name);
}

/*******************************************************************************
 *
 * Function         btm_sec_alloc_dev
 *
 * Description      Look for the record in the device database for the record
 *                  with specified address
 *
 * Returns          Pointer to the record or NULL
 *
 ******************************************************************************/
tBTM_SEC_DEV_REC* btm_sec_alloc_dev(const RawAddress& bd_addr) {
  tBTM_INQ_INFO* p_inq_info;

  tBTM_SEC_DEV_REC* p_dev_rec = btm_sec_allocate_dev_rec();

  LOG_DEBUG("Allocated device record bd_addr:%s", ADDRESS_TO_LOGGABLE_CSTR(bd_addr));

  /* Check with the BT manager if details about remote device are known */
  /* outgoing connection */
  p_inq_info = BTM_InqDbRead(bd_addr);
  if (p_inq_info != NULL) {
    memcpy(p_dev_rec->dev_class, p_inq_info->results.dev_class, DEV_CLASS_LEN);

    p_dev_rec->device_type = p_inq_info->results.device_type;
    if (is_ble_addr_type_known(p_inq_info->results.ble_addr_type))
      p_dev_rec->ble.SetAddressType(p_inq_info->results.ble_addr_type);
    else
      LOG_WARN(
          "Please do not update device record from anonymous le advertisement");

  } else if (bd_addr == btm_cb.connecting_bda)
    memcpy(p_dev_rec->dev_class, btm_cb.connecting_dc, DEV_CLASS_LEN);

  /* update conn params, use default value for background connection params */
  memset(&p_dev_rec->conn_params, 0xff, sizeof(tBTM_LE_CONN_PRAMS));

  p_dev_rec->bd_addr = bd_addr;

  p_dev_rec->ble_hci_handle = BTM_GetHCIConnHandle(bd_addr, BT_TRANSPORT_LE);
  p_dev_rec->hci_handle = BTM_GetHCIConnHandle(bd_addr, BT_TRANSPORT_BR_EDR);

  return (p_dev_rec);
}

/*******************************************************************************
 *
 * Function         btm_dev_support_role_switch
 *
 * Description      This function is called by the L2CAP to check if remote
 *                  device supports role switch
 *
 * Parameters:      bd_addr       - Address of the peer device
 *
 * Returns          true if device is known and role switch is supported
 *                  for the link.
 *
 ******************************************************************************/
bool btm_dev_support_role_switch(const RawAddress& bd_addr) {
  if (BTM_IsScoActiveByBdaddr(bd_addr)) {
    BTM_TRACE_DEBUG("%s Role switch is not allowed if a SCO is up", __func__);
    return false;
  }

  tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev(bd_addr);
  if (p_dev_rec == nullptr) {
    BTM_TRACE_DEBUG("%s Unknown address for role switch", __func__);
    return false;
  }

  if (!controller_get_interface()->supports_central_peripheral_role_switch()) {
    BTM_TRACE_DEBUG("%s Local controller does not support role switch",
                    __func__);
    return false;
  }

  if (p_dev_rec->remote_supports_hci_role_switch) {
    BTM_TRACE_DEBUG("%s Peer controller supports role switch", __func__);
    return true;
  }

  if (!p_dev_rec->remote_feature_received) {
    BTM_TRACE_DEBUG(
        "%s Unknown peer capabilities, assuming peer supports role switch",
        __func__);
    return true;
  }

  BTM_TRACE_DEBUG("%s Peer controller does not support role switch", __func__);
  return false;
}

bool is_handle_equal(void* data, void* context) {
  tBTM_SEC_DEV_REC* p_dev_rec = static_cast<tBTM_SEC_DEV_REC*>(data);
  uint16_t* handle = static_cast<uint16_t*>(context);

  if (p_dev_rec->hci_handle == *handle || p_dev_rec->ble_hci_handle == *handle)
    return false;

  return true;
}

/*******************************************************************************
 *
 * Function         btm_find_dev_by_handle
 *
 * Description      Look for the record in the device database for the record
 *                  with specified handle
 *
 * Returns          Pointer to the record or NULL
 *
 ******************************************************************************/
tBTM_SEC_DEV_REC* btm_find_dev_by_handle(uint16_t handle) {
  list_node_t* n = list_foreach(btm_cb.sec_dev_rec, is_handle_equal, &handle);
  if (n) return static_cast<tBTM_SEC_DEV_REC*>(list_node(n));

  return NULL;
}

bool is_address_equal(void* data, void* context) {
  tBTM_SEC_DEV_REC* p_dev_rec = static_cast<tBTM_SEC_DEV_REC*>(data);
  const RawAddress* bd_addr = ((RawAddress*)context);

  if (p_dev_rec->bd_addr == *bd_addr) return false;
  // If a LE random address is looking for device record
  if (p_dev_rec->ble.pseudo_addr == *bd_addr) return false;

  if (btm_ble_addr_resolvable(*bd_addr, p_dev_rec)) return false;
  return true;
}

/*******************************************************************************
 *
 * Function         btm_find_dev
 *
 * Description      Look for the record in the device database for the record
 *                  with specified BD address
 *
 * Returns          Pointer to the record or NULL
 *
 ******************************************************************************/
tBTM_SEC_DEV_REC* btm_find_dev(const RawAddress& bd_addr) {
  if (btm_cb.sec_dev_rec == nullptr) return nullptr;

  list_node_t* n =
      list_foreach(btm_cb.sec_dev_rec, is_address_equal, (void*)&bd_addr);
  if (n) return static_cast<tBTM_SEC_DEV_REC*>(list_node(n));

  return NULL;
}

static bool has_lenc_and_address_is_equal(void* data, void* context) {
  tBTM_SEC_DEV_REC* p_dev_rec = static_cast<tBTM_SEC_DEV_REC*>(data);
  if (!(p_dev_rec->ble.key_type & BTM_LE_KEY_LENC)) return true;

  return is_address_equal(data, context);
}

/*******************************************************************************
 *
 * Function         btm_find_dev_with_lenc
 *
 * Description      Look for the record in the device database with LTK and
 *                  specified BD address
 *
 * Returns          Pointer to the record or NULL
 *
 ******************************************************************************/
tBTM_SEC_DEV_REC* btm_find_dev_with_lenc(const RawAddress& bd_addr) {
  if (btm_cb.sec_dev_rec == nullptr) return nullptr;

  list_node_t* n = list_foreach(btm_cb.sec_dev_rec, has_lenc_and_address_is_equal,
                                (void*)&bd_addr);
  if (n) return static_cast<tBTM_SEC_DEV_REC*>(list_node(n));

  return NULL;
}
/*******************************************************************************
 *
 * Function         btm_consolidate_dev
5**
 * Description      combine security records if identified as same peer
 *
 * Returns          none
 *
 ******************************************************************************/
void btm_consolidate_dev(tBTM_SEC_DEV_REC* p_target_rec) {
  tBTM_SEC_DEV_REC temp_rec = *p_target_rec;

  BTM_TRACE_DEBUG("%s", __func__);

  list_node_t* end = list_end(btm_cb.sec_dev_rec);
  list_node_t* node = list_begin(btm_cb.sec_dev_rec);
  while (node != end) {
    tBTM_SEC_DEV_REC* p_dev_rec =
        static_cast<tBTM_SEC_DEV_REC*>(list_node(node));

    // we do list_remove in some cases, must grab next before removing
    node = list_next(node);

    if (p_target_rec == p_dev_rec) continue;

    if (p_dev_rec->bd_addr == p_target_rec->bd_addr) {
      memcpy(p_target_rec, p_dev_rec, sizeof(tBTM_SEC_DEV_REC));
      p_target_rec->ble = temp_rec.ble;
      p_target_rec->ble_hci_handle = temp_rec.ble_hci_handle;
      p_target_rec->enc_key_size = temp_rec.enc_key_size;
      p_target_rec->conn_params = temp_rec.conn_params;
      p_target_rec->device_type |= temp_rec.device_type;
      p_target_rec->sec_flags |= temp_rec.sec_flags;

      p_target_rec->new_encryption_key_is_p256 =
          temp_rec.new_encryption_key_is_p256;
      p_target_rec->bond_type = temp_rec.bond_type;

      /* remove the combined record */
      wipe_secrets_and_remove(p_dev_rec);
      // p_dev_rec gets freed in list_remove, we should not  access it further
      continue;
    }

    /* an RPA device entry is a duplicate of the target record */
    if (btm_ble_addr_resolvable(p_dev_rec->bd_addr, p_target_rec)) {
      if (p_target_rec->ble.pseudo_addr == p_dev_rec->bd_addr) {
        p_target_rec->ble.SetAddressType(p_dev_rec->ble.AddressType());
        p_target_rec->device_type |= p_dev_rec->device_type;

        /* remove the combined record */
        wipe_secrets_and_remove(p_dev_rec);
      }
    }
  }
}

BTM_CONSOLIDATION_CB* btm_consolidate_cb = nullptr;

void BTM_SetConsolidationCallback(BTM_CONSOLIDATION_CB* cb) {
  btm_consolidate_cb = cb;
}

/* combine security records of established LE connections after Classic pairing
 * succeeded. */
void btm_dev_consolidate_existing_connections(const RawAddress& bd_addr) {
  tBTM_SEC_DEV_REC* p_target_rec = btm_find_dev(bd_addr);
  if (!p_target_rec) {
    LOG_ERROR("No security record for just bonded device!?!?");
    return;
  }

  if (p_target_rec->ble_hci_handle != HCI_INVALID_HANDLE) {
    LOG_INFO("Not consolidating - already have LE connection");
    return;
  }

  LOG_INFO("%s", ADDRESS_TO_LOGGABLE_CSTR(bd_addr));

  list_node_t* end = list_end(btm_cb.sec_dev_rec);
  list_node_t* node = list_begin(btm_cb.sec_dev_rec);
  while (node != end) {
    tBTM_SEC_DEV_REC* p_dev_rec =
        static_cast<tBTM_SEC_DEV_REC*>(list_node(node));

    // we do list_remove in some cases, must grab next before removing
    node = list_next(node);

    if (p_target_rec == p_dev_rec) continue;

    /* an RPA device entry is a duplicate of the target record */
    if (btm_ble_addr_resolvable(p_dev_rec->bd_addr, p_target_rec)) {
      if (p_dev_rec->ble_hci_handle == HCI_INVALID_HANDLE) {
        LOG_INFO("already disconnected - erasing entry %s",
                 ADDRESS_TO_LOGGABLE_CSTR(p_dev_rec->bd_addr));
        wipe_secrets_and_remove(p_dev_rec);
        continue;
      }

      LOG_INFO(
          "Found existing LE connection to just bonded device on %s handle 0x%04x",
          ADDRESS_TO_LOGGABLE_CSTR(p_dev_rec->bd_addr), p_dev_rec->ble_hci_handle);

      RawAddress ble_conn_addr = p_dev_rec->bd_addr;
      p_target_rec->ble_hci_handle = p_dev_rec->ble_hci_handle;

      /* remove the old LE record */
      wipe_secrets_and_remove(p_dev_rec);

      btm_acl_consolidate(bd_addr, ble_conn_addr);
      L2CA_Consolidate(bd_addr, ble_conn_addr);
      gatt_consolidate(bd_addr, ble_conn_addr);
      if (btm_consolidate_cb) btm_consolidate_cb(bd_addr, ble_conn_addr);

      /* To avoid race conditions between central/peripheral starting encryption
       * at same time, initiate it just from central. */
      if (L2CA_GetBleConnRole(ble_conn_addr) == HCI_ROLE_CENTRAL) {
        LOG_INFO("Will encrypt existing connection");
        BTM_SetEncryption(bd_addr, BT_TRANSPORT_LE, nullptr, nullptr,
                          BTM_BLE_SEC_ENCRYPT);
      }
    }
  }
}

/*******************************************************************************
 *
 * Function         btm_find_or_alloc_dev
 *
 * Description      Look for the record in the device database for the record
 *                  with specified BD address
 *
 * Returns          Pointer to the record or NULL
 *
 ******************************************************************************/
tBTM_SEC_DEV_REC* btm_find_or_alloc_dev(const RawAddress& bd_addr) {
  tBTM_SEC_DEV_REC* p_dev_rec;
  BTM_TRACE_EVENT("btm_find_or_alloc_dev");
  p_dev_rec = btm_find_dev(bd_addr);
  if (p_dev_rec == NULL) {
    /* Allocate a new device record or reuse the oldest one */
    p_dev_rec = btm_sec_alloc_dev(bd_addr);
  }
  return (p_dev_rec);
}

/*******************************************************************************
 *
 * Function         btm_find_oldest_dev_rec
 *
 * Description      Locates the oldest device in use. It first looks for
 *                  the oldest non-paired device.  If all devices are paired it
 *                  returns the oldest paired device.
 *
 * Returns          Pointer to the record or NULL
 *
 ******************************************************************************/
static tBTM_SEC_DEV_REC* btm_find_oldest_dev_rec(void) {
  tBTM_SEC_DEV_REC* p_oldest = NULL;
  uint32_t ts_oldest = 0xFFFFFFFF;
  tBTM_SEC_DEV_REC* p_oldest_paired = NULL;
  uint32_t ts_oldest_paired = 0xFFFFFFFF;

  list_node_t* end = list_end(btm_cb.sec_dev_rec);
  for (list_node_t* node = list_begin(btm_cb.sec_dev_rec); node != end;
       node = list_next(node)) {
    tBTM_SEC_DEV_REC* p_dev_rec =
        static_cast<tBTM_SEC_DEV_REC*>(list_node(node));

    if ((p_dev_rec->sec_flags &
         (BTM_SEC_LINK_KEY_KNOWN | BTM_SEC_LE_LINK_KEY_KNOWN)) == 0) {
      // Device is not paired
      if (p_dev_rec->timestamp < ts_oldest) {
        p_oldest = p_dev_rec;
        ts_oldest = p_dev_rec->timestamp;
      }
    } else {
      // Paired device
      if (p_dev_rec->timestamp < ts_oldest_paired) {
        p_oldest_paired = p_dev_rec;
        ts_oldest_paired = p_dev_rec->timestamp;
      }
    }
  }

  // If we did not find any non-paired devices, use the oldest paired one...
  if (ts_oldest == 0xFFFFFFFF) p_oldest = p_oldest_paired;

  return p_oldest;
}

/*******************************************************************************
 *
 * Function         btm_sec_allocate_dev_rec
 *
 * Description      Attempts to allocate a new device record. If we have
 *                  exceeded the maximum number of allowable records to
 *                  allocate, the oldest record will be deleted to make room
 *                  for the new record.
 *
 * Returns          Pointer to the newly allocated record
 *
 ******************************************************************************/
tBTM_SEC_DEV_REC* btm_sec_allocate_dev_rec(void) {
  tBTM_SEC_DEV_REC* p_dev_rec = NULL;

  if (list_length(btm_cb.sec_dev_rec) > BTM_SEC_MAX_DEVICE_RECORDS) {
    p_dev_rec = btm_find_oldest_dev_rec();
    wipe_secrets_and_remove(p_dev_rec);
  }

  p_dev_rec =
      static_cast<tBTM_SEC_DEV_REC*>(osi_calloc(sizeof(tBTM_SEC_DEV_REC)));
  list_append(btm_cb.sec_dev_rec, p_dev_rec);

  // Initialize defaults
  p_dev_rec->sec_flags = BTM_SEC_IN_USE;
  p_dev_rec->bond_type = tBTM_SEC_DEV_REC::BOND_TYPE_UNKNOWN;
  p_dev_rec->timestamp = btm_cb.dev_rec_count++;
  p_dev_rec->rmt_io_caps = BTM_IO_CAP_UNKNOWN;
  p_dev_rec->suggested_tx_octets = 0;

  return p_dev_rec;
}

/*******************************************************************************
 *
 * Function         btm_get_bond_type_dev
 *
 * Description      Get the bond type for a device in the device database
 *                  with specified BD address
 *
 * Returns          The device bond type if known, otherwise BOND_TYPE_UNKNOWN
 *
 ******************************************************************************/
tBTM_SEC_DEV_REC::tBTM_BOND_TYPE btm_get_bond_type_dev(
    const RawAddress& bd_addr) {
  tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev(bd_addr);

  if (p_dev_rec == NULL) return tBTM_SEC_DEV_REC::BOND_TYPE_UNKNOWN;

  return p_dev_rec->bond_type;
}

/*******************************************************************************
 *
 * Function         btm_set_bond_type_dev
 *
 * Description      Set the bond type for a device in the device database
 *                  with specified BD address
 *
 * Returns          true on success, otherwise false
 *
 ******************************************************************************/
bool btm_set_bond_type_dev(const RawAddress& bd_addr,
                           tBTM_SEC_DEV_REC::tBTM_BOND_TYPE bond_type) {
  tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev(bd_addr);

  if (p_dev_rec == NULL) return false;

  p_dev_rec->bond_type = bond_type;
  return true;
}

/*******************************************************************************
 *
 * Function         btm_get_sec_dev_rec
 *
 * Description      Get security device records satisfying given filter
 *
 * Returns          A vector containing pointers of security device records
 *
 ******************************************************************************/
std::vector<tBTM_SEC_DEV_REC*> btm_get_sec_dev_rec() {
  std::vector<tBTM_SEC_DEV_REC*> result{};

  list_node_t* end = list_end(btm_cb.sec_dev_rec);
  for (list_node_t* node = list_begin(btm_cb.sec_dev_rec); node != end;
       node = list_next(node)) {
    tBTM_SEC_DEV_REC* p_dev_rec =
        static_cast<tBTM_SEC_DEV_REC*>(list_node(node));
    result.push_back(p_dev_rec);
  }
  return result;
}
