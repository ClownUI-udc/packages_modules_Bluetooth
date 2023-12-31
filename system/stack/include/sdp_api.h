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

#pragma once

#include <base/strings/stringprintf.h>

#include <cstdint>

#include "bt_target.h"
#include "sdpdefs.h"
#include "types/bluetooth/uuid.h"
#include "types/raw_address.h"

/*****************************************************************************
 *  Constants
 ****************************************************************************/

/* Success code and error codes */
typedef enum : uint16_t {
  SDP_SUCCESS = 0x0000,
  SDP_INVALID_VERSION = 0x0001,
  SDP_INVALID_SERV_REC_HDL = 0x0002,
  SDP_INVALID_REQ_SYNTAX = 0x0003,
  SDP_INVALID_PDU_SIZE = 0x0004,
  SDP_INVALID_CONT_STATE = 0x0005,
  SDP_NO_RESOURCES = 0x0006,
  SDP_DI_REG_FAILED = 0x0007,
  SDP_DI_DISC_FAILED = 0x0008,
  SDP_NO_DI_RECORD_FOUND = 0x0009,
  SDP_ERR_ATTR_NOT_PRESENT = 0x000A,
  SDP_ILLEGAL_PARAMETER = 0x000B,

  HID_SDP_NO_SERV_UUID = (SDP_ILLEGAL_PARAMETER + 1),
  HID_SDP_MANDATORY_MISSING,

  SDP_NO_RECS_MATCH = 0xFFF0,
  SDP_CONN_FAILED = 0xFFF1,
  SDP_CFG_FAILED = 0xFFF2,
  SDP_GENERIC_ERROR = 0xFFF3,
  SDP_DB_FULL = 0xFFF4,
  SDP_CANCEL = 0xFFF8,
} tSDP_STATUS;
using tSDP_RESULT = tSDP_STATUS;
using tSDP_REASON = tSDP_STATUS;

#define CASE_RETURN_TEXT(code) \
  case code:                   \
    return #code

inline std::string sdp_status_text(const tSDP_STATUS& status) {
  switch (status) {
    CASE_RETURN_TEXT(SDP_SUCCESS);
    CASE_RETURN_TEXT(SDP_INVALID_VERSION);
    CASE_RETURN_TEXT(SDP_INVALID_SERV_REC_HDL);
    CASE_RETURN_TEXT(SDP_INVALID_REQ_SYNTAX);
    CASE_RETURN_TEXT(SDP_INVALID_PDU_SIZE);
    CASE_RETURN_TEXT(SDP_INVALID_CONT_STATE);
    CASE_RETURN_TEXT(SDP_NO_RESOURCES);
    CASE_RETURN_TEXT(SDP_DI_REG_FAILED);
    CASE_RETURN_TEXT(SDP_DI_DISC_FAILED);
    CASE_RETURN_TEXT(SDP_NO_DI_RECORD_FOUND);
    CASE_RETURN_TEXT(SDP_ERR_ATTR_NOT_PRESENT);
    CASE_RETURN_TEXT(SDP_ILLEGAL_PARAMETER);

    CASE_RETURN_TEXT(HID_SDP_NO_SERV_UUID);
    CASE_RETURN_TEXT(HID_SDP_MANDATORY_MISSING);

    CASE_RETURN_TEXT(SDP_NO_RECS_MATCH);
    CASE_RETURN_TEXT(SDP_CONN_FAILED);
    CASE_RETURN_TEXT(SDP_CFG_FAILED);
    CASE_RETURN_TEXT(SDP_GENERIC_ERROR);
    CASE_RETURN_TEXT(SDP_DB_FULL);
    CASE_RETURN_TEXT(SDP_CANCEL);
    default:
      return base::StringPrintf("UNKNOWN[%hu]", status);
  }
}
const auto sdp_result_text = sdp_status_text;

#undef CASE_RETURN_TEXT

/* Masks for attr_value field of tSDP_DISC_ATTR */
#define SDP_DISC_ATTR_LEN_MASK 0x0FFF
#define SDP_DISC_ATTR_TYPE(len_type) ((len_type) >> 12)
#define SDP_DISC_ATTR_LEN(len_type) ((len_type)&SDP_DISC_ATTR_LEN_MASK)

/* Maximum number of protocol list items (list_elem in tSDP_PROTOCOL_ELEM) */
#define SDP_MAX_LIST_ELEMS 3

/*****************************************************************************
 *  Type Definitions
 ****************************************************************************/

/* Define a callback function for when discovery is complete. */
typedef void(tSDP_DISC_CMPL_CB)(tSDP_RESULT result);
typedef void(tSDP_DISC_CMPL_CB2)(tSDP_RESULT result, const void* user_data);

typedef struct {
  RawAddress peer_addr;
  uint16_t peer_mtu;
} tSDP_DR_OPEN;

typedef struct {
  uint8_t* p_data;
  uint16_t data_len;
} tSDP_DR_DATA;

typedef union {
  tSDP_DR_OPEN open;
  tSDP_DR_DATA data;
} tSDP_DATA;

/* Define a structure to hold the discovered service information. */
typedef struct {
  union {
    uint8_t u8;                         /* 8-bit integer            */
    uint16_t u16;                       /* 16-bit integer           */
    uint32_t u32;                       /* 32-bit integer           */
    struct t_sdp_disc_attr* p_sub_attr; /* Addr of first sub-attr (list)*/
    uint8_t array[];                    /* Variable length field    */
                                        /* flexible array member    */
                                        /* requiring backing store  */
                                        /* from SDP DB    */
  } v;

} tSDP_DISC_ATVAL;

typedef struct t_sdp_disc_attr {
  struct t_sdp_disc_attr* p_next_attr; /* Addr of next linked attr     */
  uint16_t attr_id;                    /* Attribute ID                 */
  uint16_t attr_len_type;              /* Length and type fields       */
  tSDP_DISC_ATVAL attr_value;          /* Variable length entry data   */
} tSDP_DISC_ATTR;

typedef struct t_sdp_disc_rec {
  tSDP_DISC_ATTR* p_first_attr;      /* First attribute of record    */
  struct t_sdp_disc_rec* p_next_rec; /* Addr of next linked record   */
  uint32_t time_read;                /* The time the record was read */
  RawAddress remote_bd_addr;         /* Remote BD address            */
} tSDP_DISC_REC;

typedef struct {
  uint32_t mem_size;          /* Memory size of the DB        */
  uint32_t mem_free;          /* Memory still available       */
  tSDP_DISC_REC* p_first_rec; /* Addr of first record in DB   */
  uint16_t num_uuid_filters;  /* Number of UUIds to filter    */
  bluetooth::Uuid uuid_filters[SDP_MAX_UUID_FILTERS]; /* UUIDs to filter */
  uint16_t num_attr_filters; /* Number of attribute filters  */
  uint16_t attr_filters[SDP_MAX_ATTR_FILTERS]; /* Attributes to filter */
  uint8_t* p_free_mem; /* Pointer to free memory       */
  uint8_t*
      raw_data; /* Received record from server. allocated/released by client  */
  uint32_t raw_size; /* size of raw_data */
  uint32_t raw_used; /* length of raw_data used */
} tSDP_DISCOVERY_DB;

/* This structure is used to add protocol lists and find protocol elements */
typedef struct {
  uint16_t protocol_uuid;
  uint16_t num_params;
  uint16_t params[SDP_MAX_PROTOCOL_PARAMS];
} tSDP_PROTOCOL_ELEM;

typedef struct {
  uint16_t num_elems;
  tSDP_PROTOCOL_ELEM list_elem[SDP_MAX_LIST_ELEMS];
} tSDP_PROTO_LIST_ELEM;

/* Device Identification (DI) data structure
*/
/* Used to set the DI record */
typedef struct t_sdp_di_record {
  uint16_t vendor;
  uint16_t vendor_id_source;
  uint16_t product;
  uint16_t version;
  bool primary_record;
  char client_executable_url[SDP_MAX_ATTR_LEN]; /* optional */
  char service_description[SDP_MAX_ATTR_LEN];   /* optional */
  char documentation_url[SDP_MAX_ATTR_LEN];     /* optional */
} tSDP_DI_RECORD;

/* Used to get the DI record */
typedef struct t_sdp_di_get_record {
  uint16_t spec_id;
  tSDP_DI_RECORD rec;
} tSDP_DI_GET_RECORD;

/* API into the SDP layer for service discovery. */

/*******************************************************************************
 *
 * Function         SDP_InitDiscoveryDb
 *
 * Description      This function is called to initialize a discovery database.
 *
 * Returns          true if successful, false if one or more parameters are bad
 *
 ******************************************************************************/
bool SDP_InitDiscoveryDb(tSDP_DISCOVERY_DB* p_db, uint32_t len,
                         uint16_t num_uuid, const bluetooth::Uuid* p_uuid_list,
                         uint16_t num_attr, const uint16_t* p_attr_list);

/*******************************************************************************
 *
 * Function         SDP_CancelServiceSearch
 *
 * Description      This function cancels an active query to an SDP server.
 *
 * Returns          true if discovery cancelled, false if a matching activity is
 *                  not found.
 *
 ******************************************************************************/
bool SDP_CancelServiceSearch(const tSDP_DISCOVERY_DB* p_db);

/*******************************************************************************
 *
 * Function         SDP_ServiceSearchRequest
 *
 * Description      This function queries an SDP server for information.
 *
 * Returns          true if discovery started, false if failed.
 *
 ******************************************************************************/
bool SDP_ServiceSearchRequest(const RawAddress& p_bd_addr,
                              tSDP_DISCOVERY_DB* p_db, tSDP_DISC_CMPL_CB* p_cb);

/*******************************************************************************
 *
 * Function         SDP_ServiceSearchAttributeRequest
 *
 * Description      This function queries an SDP server for information.
 *
 *                  The difference between this API function and the function
 *                  SDP_ServiceSearchRequest is that this one does a
 *                  combined ServiceSearchAttributeRequest SDP function.
 *
 * Returns          true if discovery started, false if failed.
 *
 ******************************************************************************/
bool SDP_ServiceSearchAttributeRequest(const RawAddress& p_bd_addr,
                                       tSDP_DISCOVERY_DB* p_db,
                                       tSDP_DISC_CMPL_CB* p_cb);

/*******************************************************************************
 *
 * Function         SDP_ServiceSearchAttributeRequest2
 *
 * Description      This function queries an SDP server for information.
 *
 *                  The difference between this API function and the function
 *                  SDP_ServiceSearchRequest is that this one does a
 *                  combined ServiceSearchAttributeRequest SDP function with the
 *                  user data piggyback
 *
 * Returns          true if discovery started, false if failed.
 *
 ******************************************************************************/
bool SDP_ServiceSearchAttributeRequest2(const RawAddress& p_bd_addr,
                                        tSDP_DISCOVERY_DB* p_db,
                                        tSDP_DISC_CMPL_CB2* p_cb,
                                        const void* user_data);

/* API of utilities to find data in the local discovery database */

/*******************************************************************************
 *
 * Function         SDP_FindAttributeInRec
 *
 * Description      This function searches an SDP discovery record for a
 *                  specific attribute.
 *
 * Returns          Pointer to matching attribute entry, or NULL
 *
 ******************************************************************************/
tSDP_DISC_ATTR* SDP_FindAttributeInRec(const tSDP_DISC_REC* p_rec,
                                       uint16_t attr_id);

/*******************************************************************************
 *
 * Function         SDP_FindServiceInDb
 *
 * Description      This function queries an SDP database for a specific
 *                  service. If the p_start_rec pointer is NULL, it looks from
 *                  the beginning of the database, else it continues from the
 *                  next record after p_start_rec.
 *
 * Returns          Pointer to record containing service class, or NULL
 *
 ******************************************************************************/
tSDP_DISC_REC* SDP_FindServiceInDb(const tSDP_DISCOVERY_DB* p_db,
                                   uint16_t service_uuid,
                                   tSDP_DISC_REC* p_start_rec);

/*******************************************************************************
 *
 * Function         SDP_FindServiceUUIDInDb
 *
 * Description      This function queries an SDP database for a specific
 *                  service. If the p_start_rec pointer is NULL, it looks from
 *                  the beginning of the database, else it continues from the
 *                  next record after p_start_rec.
 *
 * NOTE             the only difference between this function and the previous
 *                  function "SDP_FindServiceInDb()" is that this function takes
 *                  a Uuid input.
 *
 * Returns          Pointer to record containing service class, or NULL
 *
 ******************************************************************************/
tSDP_DISC_REC* SDP_FindServiceUUIDInDb(const tSDP_DISCOVERY_DB* p_db,
                                       const bluetooth::Uuid& uuid,
                                       tSDP_DISC_REC* p_start_rec);

/*******************************************************************************
 *
 * Function         SDP_FindServiceUUIDInRec_128bit
 *
 * Description      Read the 128-bit service UUID within a record,
 *                  if there is any.
 *
 * Parameters:      p_rec      - pointer to a SDP record.
 *                  p_uuid     - output parameter to save the UUID found.
 *
 * Returns          true if found, otherwise false.
 *
 ******************************************************************************/
bool SDP_FindServiceUUIDInRec_128bit(const tSDP_DISC_REC* p_rec,
                                     bluetooth::Uuid* p_uuid);

/*******************************************************************************
 *
 * Function         SDP_FindServiceInDb_128bit
 *
 * Description      Query an SDP database for a specific service.
 *                  If the p_start_rec pointer is NULL, look from the beginning
 *                  of the database, else continue from the next record after
 *                  p_start_rec.
 *
 * Returns          Pointer to record containing service class, or NULL
 *
 ******************************************************************************/
tSDP_DISC_REC* SDP_FindServiceInDb_128bit(const tSDP_DISCOVERY_DB* p_db,
                                          tSDP_DISC_REC* p_start_rec);

/*******************************************************************************
 *
 * Function         SDP_FindProtocolListElemInRec
 *
 * Description      This function looks at a specific discovery record for a
 *                  protocol list element.
 *
 * Returns          true if found, false if not
 *                  If found, the passed protocol list element is filled in.
 *
 ******************************************************************************/
bool SDP_FindProtocolListElemInRec(const tSDP_DISC_REC* p_rec,
                                   uint16_t layer_uuid,
                                   tSDP_PROTOCOL_ELEM* p_elem);

/*******************************************************************************
 *
 * Function         SDP_FindProfileVersionInRec
 *
 * Description      This function looks at a specific discovery record for the
 *                  Profile list descriptor, and pulls out the version number.
 *                  The version number consists of an 8-bit major version and
 *                  an 8-bit minor version.
 *
 * Returns          true if found, false if not
 *                  If found, the major and minor version numbers that were
 *                  passed in are filled in.
 *
 ******************************************************************************/
bool SDP_FindProfileVersionInRec(const tSDP_DISC_REC* p_rec,
                                 uint16_t profile_uuid, uint16_t* p_version);

/* API into SDP for local service database updates */

/*******************************************************************************
 *
 * Function         SDP_CreateRecord
 *
 * Description      This function is called to create a record in the database.
 *                  This would be through the SDP database maintenance API. The
 *                  record is created empty, teh application should then call
 *                  "add_attribute" to add the record's attributes.
 *
 * Returns          Record handle if OK, else 0.
 *
 ******************************************************************************/
uint32_t SDP_CreateRecord(void);

/*******************************************************************************
 *
 * Function         SDP_DeleteRecord
 *
 * Description      This function is called to add a record (or all records)
 *                  from the database. This would be through the SDP database
 *                  maintenance API.
 *
 *                  If a record handle of 0 is passed, all records are deleted.
 *
 * Returns          true if succeeded, else false
 *
 ******************************************************************************/
bool SDP_DeleteRecord(uint32_t handle);

/*******************************************************************************
 *
 * Function         SDP_AddAttribute
 *
 * Description      This function is called to add an attribute to a record.
 *                  This would be through the SDP database maintenance API.
 *                  If the attribute already exists in the record, it is
 *                  replaced with the new value.
 *
 * NOTE             Attribute values must be passed as a Big Endian stream.
 *
 * Returns          true if added OK, else false
 *
 ******************************************************************************/
bool SDP_AddAttribute(uint32_t handle, uint16_t attr_id, uint8_t attr_type,
                      uint32_t attr_len, uint8_t* p_val);

/*******************************************************************************
 *
 * Function         SDP_AddSequence
 *
 * Description      This function is called to add a sequence to a record.
 *                  This would be through the SDP database maintenance API.
 *                  If the sequence already exists in the record, it is replaced
 *                  with the new sequence.
 *
 * NOTE             Element values must be passed as a Big Endian stream.
 *
 * Returns          true if added OK, else false
 *
 ******************************************************************************/
bool SDP_AddSequence(uint32_t handle, uint16_t attr_id, uint16_t num_elem,
                     uint8_t type[], uint8_t len[], uint8_t* p_val[]);

/*******************************************************************************
 *
 * Function         SDP_AddUuidSequence
 *
 * Description      This function is called to add a UUID sequence to a record.
 *                  This would be through the SDP database maintenance API.
 *                  If the sequence already exists in the record, it is replaced
 *                  with the new sequence.
 *
 * Returns          true if added OK, else false
 *
 ******************************************************************************/
bool SDP_AddUuidSequence(uint32_t handle, uint16_t attr_id, uint16_t num_uuids,
                         uint16_t* p_uuids);

/*******************************************************************************
 *
 * Function         SDP_AddProtocolList
 *
 * Description      This function is called to add a protocol descriptor list to
 *                  a record. This would be through the SDP database
 *                  maintenance API. If the protocol list already exists in the
 *                  record, it is replaced with the new list.
 *
 * Returns          true if added OK, else false
 *
 ******************************************************************************/
bool SDP_AddProtocolList(uint32_t handle, uint16_t num_elem,
                         tSDP_PROTOCOL_ELEM* p_elem_list);

/*******************************************************************************
 *
 * Function         SDP_AddAdditionProtoLists
 *
 * Description      This function is called to add a protocol descriptor list to
 *                  a record. This would be through the SDP database maintenance
 *                  API. If the protocol list already exists in the record, it
 *                  is replaced with the new list.
 *
 * Returns          true if added OK, else false
 *
 ******************************************************************************/
bool SDP_AddAdditionProtoLists(uint32_t handle, uint16_t num_elem,
                               tSDP_PROTO_LIST_ELEM* p_proto_list);

/*******************************************************************************
 *
 * Function         SDP_AddProfileDescriptorList
 *
 * Description      This function is called to add a profile descriptor list to
 *                  a record. This would be through the SDP database maintenance
 *                  API. If the version already exists in the record, it is
 *                  replaced with the new one.
 *
 * Returns          true if added OK, else false
 *
 ******************************************************************************/
bool SDP_AddProfileDescriptorList(uint32_t handle, uint16_t profile_uuid,
                                  uint16_t version);

/*******************************************************************************
 *
 * Function         SDP_AddLanguageBaseAttrIDList
 *
 * Description      This function is called to add a language base attr list to
 *                  a record. This would be through the SDP database maintenance
 *                  API. If the version already exists in the record, it is
 *                  replaced with the new one.
 *
 * Returns          true if added OK, else false
 *
 ******************************************************************************/
bool SDP_AddLanguageBaseAttrIDList(uint32_t handle, uint16_t lang,
                                   uint16_t char_enc, uint16_t base_id);

/*******************************************************************************
 *
 * Function         SDP_AddServiceClassIdList
 *
 * Description      This function is called to add a service list to a record.
 *                  This would be through the SDP database maintenance API.
 *                  If the service list already exists in the record, it is
 *                  replaced with the new list.
 *
 * Returns          true if added OK, else false
 *
 ******************************************************************************/
bool SDP_AddServiceClassIdList(uint32_t handle, uint16_t num_services,
                               uint16_t* p_service_uuids);

/*******************************************************************************
 *
 * Function         SDP_DeleteAttribute
 *
 * Description      Delete an attribute from a record.
 *                  This would be through the SDP database maintenance API.
 *
 * Returns          true if deleted OK, else false if not found
 *
 ******************************************************************************/
bool SDP_DeleteAttribute(uint32_t handle, uint16_t attr_id);

/* Device Identification APIs */

/*******************************************************************************
 *
 * Function         SDP_SetLocalDiRecord
 *
 * Description      This function adds a DI record to the local SDP database.
 *
 * Returns          Returns SDP_SUCCESS if record added successfully, else error
 *
 ******************************************************************************/
uint16_t SDP_SetLocalDiRecord(const tSDP_DI_RECORD* device_info,
                              uint32_t* p_handle);

/*******************************************************************************
 *
 * Function         SDP_DiDiscover
 *
 * Description      This function queries a remote device for DI information.
 *
 * Returns          SDP_SUCCESS if query started successfully, else error
 *
 ******************************************************************************/
tSDP_STATUS SDP_DiDiscover(const RawAddress& remote_device,
                           tSDP_DISCOVERY_DB* p_db, uint32_t len,
                           tSDP_DISC_CMPL_CB* p_cb);

/*******************************************************************************
 *
 * Function         SDP_GetNumDiRecords
 *
 * Description      Searches specified database for DI records
 *
 * Returns          number of DI records found
 *
 ******************************************************************************/
uint8_t SDP_GetNumDiRecords(const tSDP_DISCOVERY_DB* p_db);

/*******************************************************************************
 *
 * Function         SDP_GetDiRecord
 *
 * Description      This function retrieves a remote device's DI record from
 *                  the specified database.
 *
 * Returns          SDP_SUCCESS if record retrieved, else error
 *
 ******************************************************************************/
uint16_t SDP_GetDiRecord(uint8_t getRecordIndex,
                         tSDP_DI_GET_RECORD* device_info,
                         const tSDP_DISCOVERY_DB* p_db);

/*******************************************************************************
 *
 * Function         SDP_SetTraceLevel
 *
 * Description      This function sets the trace level for SDP. If called with
 *                  a value of 0xFF, it simply reads the current trace level.
 *
 * Returns          the new (current) trace level
 *
 ******************************************************************************/
uint8_t SDP_SetTraceLevel(uint8_t new_level);

/*******************************************************************************
 *
 * Function         SDP_FindServiceUUIDInRec
 *
 * Description      Read the service UUID within a record,
 *                  if there is any.
 *
 * Parameters:      p_rec  - pointer to a SDP record.
 *                  p_uuid - pointer to a UUID
 *
 * Returns          true if found, otherwise false.
 *
 ******************************************************************************/
bool SDP_FindServiceUUIDInRec(const tSDP_DISC_REC* p_rec,
                              bluetooth::Uuid* p_uuid);

namespace bluetooth {
namespace legacy {
namespace stack {
namespace sdp {

struct tSdpApi {
  struct {
    /*******************************************************************************
      Function         SDP_InitDiscoveryDb

      Description      This function is called to initialize a discovery
                       database.

      Parameters:      p_db        - (input) address of an area of memory where
                                             the discovery database is managed.
                       len         - (input) size (in bytes) of the memory
                        NOTE: This must be larger than sizeof(tSDP_DISCOVERY_DB)
                       num_uuid    - (input) number of UUID filters applied
                       p_uuid_list - (input) list of UUID filters
                       num_attr    - (input) number of attribute filters
                                             applied
                       p_attr_list - (input) list of attribute filters

      Returns          true if successful, false if one or more parameters are
                       bad
     ******************************************************************************/
    bool (*SDP_InitDiscoveryDb)(tSDP_DISCOVERY_DB*, uint32_t, uint16_t,
                                const bluetooth::Uuid*, uint16_t,
                                const uint16_t*);

    /*******************************************************************************

      Function         SDP_CancelServiceSearch

      Description      This function cancels an active query to an SDP server.

      Parameters:      p_db        - (input) address of an area of memory where
                                             the discovery database is managed.

      Returns          true if discovery cancelled, false if a matching
                       activity is not found.

     ******************************************************************************/
    bool (*SDP_CancelServiceSearch)(const tSDP_DISCOVERY_DB*);

    /*******************************************************************************

      Function         SDP_ServiceSearchRequest

      Description      This function queries an SDP server for information.

      Parameters:      p_db        - (input) address of an area of memory where
                                             the discovery database is managed.
                       p_cb        - (input) callback executed when complete

      Returns          true if discovery started, false if failed.

     ******************************************************************************/
    bool (*SDP_ServiceSearchRequest)(const RawAddress&, tSDP_DISCOVERY_DB*,
                                     tSDP_DISC_CMPL_CB*);

    /*******************************************************************************

      Function         SDP_ServiceSearchAttributeRequest

      Description      This function queries an SDP server for information.

                       The difference between this API function and the
                       function SDP_ServiceSearchRequest is that this one does
                       a combined ServiceSearchAttributeRequest SDP function.

      Parameters:      bd_addr     - (input) device address for service search
                       p_db        - (input) address of an area of memory where
                                             the discovery database is managed.
                       p_cb        - (input) callback executed when complete

      Returns          true if discovery started, false if failed.

     ******************************************************************************/
    bool (*SDP_ServiceSearchAttributeRequest)(const RawAddress&,
                                              tSDP_DISCOVERY_DB*,
                                              tSDP_DISC_CMPL_CB*);

    /*******************************************************************************

      Function         SDP_ServiceSearchAttributeRequest2

      Description      This function queries an SDP server for information.

                       The difference between this API function and the
                       function SDP_ServiceSearchRequest is that this one does
                       a combined ServiceSearchAttributeRequest SDP function
                       with the user data piggyback

      parameters:      bd_addr     - (input) device address for service search
                       p_db        - (input) address of an area of memory where
                                             the discovery database is managed.
                       p_cb2       - (input) callback executed when complete
                       p_data      - (input) user data

      Returns          true if discovery started, false if failed.

     ******************************************************************************/
    bool (*SDP_ServiceSearchAttributeRequest2)(const RawAddress&,
                                               tSDP_DISCOVERY_DB*,
                                               tSDP_DISC_CMPL_CB2*,
                                               const void*);
  } service;

  struct {
    /*******************************************************************************

      Function         SDP_FindServiceInDb

      Description      This function queries an SDP database for a specific
                       service. If the p_start_rec pointer is NULL, it looks
                       from the beginning of the database, else it continues
                       from the next record after p_start_rec.

      parameters:      p_db        - (input) address of an area of memory where
                                             the discovery database is managed.
                       uuid16      - (input) Uuid to search in db
                       disc_rec    - (output) Record found, null otherwise

      Returns          Pointer to record containing service class, or NULL

     ******************************************************************************/
    tSDP_DISC_REC* (*SDP_FindServiceInDb)(const tSDP_DISCOVERY_DB*, uint16_t,
                                          tSDP_DISC_REC*);

    /*******************************************************************************

      Function         SDP_FindServiceUUIDInDb

      Description      This function queries an SDP database for a specific
                       service. If the p_start_rec pointer is NULL, it looks
                       from the beginning of the database, else it continues
                       from the next record after p_start_rec.

      NOTE             the only difference between this function and the
                       previous function "SDP_FindServiceInDb()" is that this
                       function takes a Uuid input.

      parameters:      p_db        - (input) address of an area of memory where
                                             the discovery database is managed.
                       uuid        - (input) Uuid to search in db
                       disc_rec    - (input) Start record, null from beginning

      Returns          Pointer to record containing service class, or NULL

     ******************************************************************************/
    tSDP_DISC_REC* (*SDP_FindServiceUUIDInDb)(const tSDP_DISCOVERY_DB*,
                                              const bluetooth::Uuid&,
                                              tSDP_DISC_REC*);

    /*******************************************************************************

      Function         SDP_FindServiceInDb_128bit

      Description      Query an SDP database for a specific service.
                       If the p_start_rec pointer is NULL, look from the
                       beginning of the database, else continue from the next
                       record after p_start_rec.

      parameters:      p_db        - (input) address of an area of memory where
                                             the discovery database is managed.
                       disc_rec    - (input) Start record, null from beginning

      Returns          Pointer to record containing service class, or NULL

     ******************************************************************************/
    tSDP_DISC_REC* (*SDP_FindServiceInDb_128bit)(const tSDP_DISCOVERY_DB*,
                                                 tSDP_DISC_REC*);
  } db;

  struct {
    /*******************************************************************************

      Local discovery database API

      Function         SDP_FindAttributeInRec

      Description      This function searches an SDP discovery record for a
                       specific attribute.

      parameters:      disc_rec    - (input) Start record must not be null
                       attr_id     - (input) Attribute id to search

      Returns          Pointer to matching attribute entry, or NULL

     ******************************************************************************/
    tSDP_DISC_ATTR* (*SDP_FindAttributeInRec)(const tSDP_DISC_REC*, uint16_t);

    /*******************************************************************************

      Function         SDP_FindServiceUUIDInRec_128bit

      Description      Read the 128-bit service UUID within a record;
                       if there is any.

      Parameters:      p_rec      - (input) pointer to a SDP record.
                       p_uuid     - (output) parameter to save the UUID found.

      Returns          true if found, otherwise false.

     ******************************************************************************/
    bool (*SDP_FindServiceUUIDInRec_128bit)(const tSDP_DISC_REC*,
                                            bluetooth::Uuid*);

    /*******************************************************************************

      Function         SDP_FindProtocolListElemInRec

      Description      This function looks at a specific discovery record for a
                       protocol list element.

      Parameters:      p_rec      - (input) pointer to a SDP record.
                       p_uuid     - (input) layer UUID.
                       p_elem     - (output) protocol element

      Returns          true if found, false if not
                       If found, the passed protocol list element is filled in.

     ******************************************************************************/
    bool (*SDP_FindProtocolListElemInRec)(const tSDP_DISC_REC*, uint16_t,
                                          tSDP_PROTOCOL_ELEM*);

    /*******************************************************************************

      Function         SDP_FindProfileVersionInRec

      Description      This function looks at a specific discovery record for
                       the Profile list descriptor, and pulls out the version
                       number. The version number consists of an 8-bit major
                       version and an 8-bit minor version.

      Parameters:      p_rec      - (input) pointer to a SDP record.
                       p_uuid     - (input) profile UUID.
                       p_elem     - (output) major and minor version numbers

      Returns          true if found, false if not

     ******************************************************************************/
    bool (*SDP_FindProfileVersionInRec)(const tSDP_DISC_REC*, uint16_t,
                                        uint16_t*);

    /*******************************************************************************

      Function         SDP_FindServiceUUIDInRec

      Description      Read the service UUID within a record;
                       if there is any.

      Parameters:      p_rec      - (input) pointer to a SDP record.
                       p_uuid     - (output) found UUID or null.

      Returns          true if found, otherwise false.

     ******************************************************************************/
    bool (*SDP_FindServiceUUIDInRec)(const tSDP_DISC_REC* p_rec,
                                     bluetooth::Uuid* p_uuid);
  } record;

  struct {
    /*******************************************************************************

      API into SDP for Local service database updates

      Function         SDP_CreateRecord

      Description      This function is called to create a record in the
                       database. This would be through the SDP database
                       maintenance API. The record is created empty, teh
                       application should then call "add_attribute" *to add
                       the record's attributes.

      Returns          Record handle if OK, else 0.

     ******************************************************************************/
    uint32_t (*SDP_CreateRecord)(void);

    /*******************************************************************************

      Function         SDP_DeleteRecord

      Description      This function is called to add a record (or all records)
                       from the database. This would be through the SDP
                       database maintenance API.

      Parameters:      handle     - (input) Handle to delete, 0 for all records
                                            to be deleted

      Returns          true if succeeded, else false

     ******************************************************************************/
    bool (*SDP_DeleteRecord)(uint32_t);

    /*******************************************************************************

      Function         SDP_AddAttribute

      Description      This function is called to add an attribute to a record.
                       This would be through the SDP database maintenance API.
                       If the attribute already exists in the record, it is
                       replaced with the new value.

      NOTE             Attribute values must be passed as a Big Endian stream.

      Parameters:      handle     - (input) Handle to add
                       attr_id    - (input) Attribute id to add
                       attr_type  - (input) Attribute type to add
                       attr_len   - (input) Attribute data length
                       p_val      - (input) Attribute data value

      Returns          true if added OK, else false

     ******************************************************************************/
    bool (*SDP_AddAttribute)(uint32_t handle, uint16_t attr_id,
                             uint8_t attr_type, uint32_t attr_len,
                             uint8_t* p_val);

    /*******************************************************************************

      Function         SDP_AddSequence

      Description      This function is called to add a sequence to a record.
                       This would be through the SDP database maintenance API.
                       If the sequence already exists in the record, it is
                       replaced with the new sequence.

      NOTE             Element values must be passed as a Big Endian stream.

      Parameters:      handle     - (input) Handle to add
                       attr_id    - (input) Attribute id to add
                       num_elem   - (input) Number of elements in array
                       type[]     - (input) Element type
                       len[]      - (input) Element data length
                       p_val[]    - (input) Element data value

      Returns          true if added OK, else false

     ******************************************************************************/
    bool (*SDP_AddSequence)(uint32_t handle, uint16_t attr_id,
                            uint16_t num_elem, uint8_t type[], uint8_t len[],
                            uint8_t* p_val[]);

    /*******************************************************************************

      Function         SDP_AddUuidSequence

      Description      This function is called to add a UUID sequence to a
                       record. This would be through the SDP database
                       maintenance API. If the sequence already exists in the
                      record, it is replaced with the new sequence.

      Parameters:      handle     - (input) Handle to add
                       attr_id    - (input) Attribute id to add
                       num_uuids  - (input) Number of uuids in array
                       p_uuids[]  - (input) Array uuid

      Returns          true if added OK, else false

     ******************************************************************************/
    bool (*SDP_AddUuidSequence)(uint32_t handle, uint16_t attr_id,
                                uint16_t num_uuids, uint16_t* p_uuids);

    /*******************************************************************************

      Function         SDP_AddProtocolList

      Description      This function is called to add a protocol descriptor
                       list to a record. This would be through the SDP database
                       maintenance API. If the protocol list already exists in
                       the record, it is replaced with the new list.

      Parameters:      handle     - (input) Handle to add
                       num_elem   - (input) Number of elements to add
                       elem_list[]- (input) Element data list to add

      Returns          true if added OK, else false

     ******************************************************************************/
    bool (*SDP_AddProtocolList)(uint32_t handle, uint16_t num_elem,
                                tSDP_PROTOCOL_ELEM* p_elem_list);

    /*******************************************************************************

      Function         SDP_AddAdditionProtoLists

      Description      This function is called to add a protocol descriptor
                       list to a record. This would be through the SDP database
                       maintenance API. If the protocol list already exists in
                       the record, it is replaced with the new list.

      Parameters:      handle     - (input) Handle to add
                       num_elem   - (input) Number of elements to add
                       proto_list[]- (input) Element data list to add

      Returns          true if added OK, else false

     ******************************************************************************/
    bool (*SDP_AddAdditionProtoLists)(uint32_t handle, uint16_t num_elem,
                                      tSDP_PROTO_LIST_ELEM* p_proto_list);

    /*******************************************************************************

      Function         SDP_AddProfileDescriptorList

      Description      This function is called to add a profile descriptor list
                       to a record. This would be through the SDP database
                       maintenance API. If the version already exists in the
                       record, it is replaced with the new one.

      Parameters:      handle     - (input) Handle to add
                       uuid       - (input) Uuid to add
                       version    - (input) major and minor version

      Returns          true if added OK, else false

     ******************************************************************************/
    bool (*SDP_AddProfileDescriptorList)(uint32_t handle, uint16_t profile_uuid,
                                         uint16_t version);

    /*******************************************************************************

      Function         SDP_AddLanguageBaseAttrIDList

      Description      This function is called to add a language base attr list
                       to a record. This would be through the SDP database
                       maintenance API. If the version already exists in the
                       record, it is replaced with the new one.

      Parameters:      handle     - (input) Handle to add
                       lang       - (input) language base descriptor
                       char_enc   - (input) character encoding
                       base_id    - (input) base id

      Returns          true if added OK, else false

     ******************************************************************************/
    bool (*SDP_AddLanguageBaseAttrIDList)(uint32_t handle, uint16_t lang,
                                          uint16_t char_enc, uint16_t base_id);

    /*******************************************************************************

      Function         SDP_AddServiceClassIdList

      Description      This function is called to add a service list to a
                       record. This would be through the SDP database
                       maintenance API. If the service list already exists in
                       the record, it is replaced with the new list.

      Parameters:      handle       - (input) Handle to add
                       num_services - (input) number of services to add
                       uuids[]      - (input) list of service uuids to add

      Returns          true if added OK, else false

     ******************************************************************************/
    bool (*SDP_AddServiceClassIdList)(uint32_t handle, uint16_t num_services,
                                      uint16_t* p_service_uuids);

    /*******************************************************************************

      Function         SDP_DeleteAttribute

      Description      Delete an attribute from a record.
                       This would be through the SDP database maintenance API.

      Parameters:      handle       - (input) Handle to add
                       attr_id      - (input) attribute id to delete

      Returns          true if deleted OK, else false if not found

     ******************************************************************************/
    bool (*SDP_DeleteAttribute)(uint32_t handle, uint16_t attr_id);
  } handle;

  struct {
    /*******************************************************************************

      Device Identification API

      Function         SDP_SetLocalDiRecord

      Description      This function adds a DI record to the local SDP
                       database.

      Parameters:      info         - (input) device identification record
                       p_handle     - (output) handle of record if successful

      Returns          Returns SDP_SUCCESS if record added successfully, else
                       error

     ******************************************************************************/
    uint16_t (*SDP_SetLocalDiRecord)(const tSDP_DI_RECORD* device_info,
                                     uint32_t* p_handle);

    /*******************************************************************************

      Device Identification API

      Function         SDP_DiDiscover

      Description      This function queries a remote device for DI
                       information.

      Parameters:      bd_addr      - (input) remote device
                       p_db         - (input) dicovery database
                       len          - (input ) data base length
                       p_cb         - (input) callback when complete

      Returns          SDP_SUCCESS if query started successfully, else error

     ******************************************************************************/
    tSDP_STATUS (*SDP_DiDiscover)(const RawAddress& remote_device,
                                  tSDP_DISCOVERY_DB* p_db, uint32_t len,
                                  tSDP_DISC_CMPL_CB* p_cb);

    /*******************************************************************************

      Device Identification API

      Function         SDP_GetNumDiRecords

      Description      Searches specified database for DI records

      Parameters:      p_db         - (input) dicovery database

      Returns          number of DI records found

     ******************************************************************************/
    uint8_t (*SDP_GetNumDiRecords)(const tSDP_DISCOVERY_DB* p_db);

    /*******************************************************************************

      Device Identification API

      Function         SDP_GetDiRecord

      Description      This function retrieves a remote device's DI record from
                       the specified database.

      Parameters:      index        - (input) record index to retrieve
                       device_info  - (input) dicovery database
                       p_cb         - (input) callback when complete

      Returns          SDP_SUCCESS if record retrieved, else error

     ******************************************************************************/
    uint16_t (*SDP_GetDiRecord)(uint8_t getRecordIndex,
                                tSDP_DI_GET_RECORD* device_info,
                                const tSDP_DISCOVERY_DB* p_db);

  } device_id;
};

const struct tSdpApi* get_legacy_stack_sdp_api();

struct tLegacyStackSdbCallback {
  void(tSDP_DISC_CMPL_CB)(tSDP_RESULT result);
  void(tSDP_DISC_CMPL_CB2)(tSDP_RESULT result, const void* user_data);
};

}  // namespace sdp
}  // namespace stack
}  // namespace legacy
}  // namespace bluetooth
