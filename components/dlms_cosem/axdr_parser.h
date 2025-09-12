#pragma once

#include <cstdint>
#include <string>
#include <functional>

#include <client.h>
#include <converters.h>
#include <cosem.h>
#include <dlmssettings.h>

const char *dlms_data_type_to_string(DLMS_DATA_TYPE vt);
const char * dlms_error_to_string(int error);
//int hlp_getDataTypeSize(DLMS_DATA_TYPE type);
bool hlp_isValueDataType(DLMS_DATA_TYPE type);

// typedef struct {
//   unsigned char *data;
//   uint32_t capacity;
//   uint32_t size;
//   uint32_t position;

// } gxByteBuffer;

// xDLMS APDU parsing helpers

// Callback function for attribute descriptor found
// Parameters: class_id, obis_code, attribute_id, value_type, value_buffer_ptr, value_length
using AttributeDescriptorCallback =
    std::function<void(uint16_t, const uint8_t *, uint8_t, DLMS_DATA_TYPE, const uint8_t *, uint8_t)>;

class AxdrParserFast {
  gxByteBuffer *buffer;
  const std::string tag_{"AxdrStreamParser"};
  AttributeDescriptorCallback callback_;
  size_t objects_found = 0;

  uint8_t peek_byte();
  uint8_t read_byte();
  uint16_t read_u16();
  uint32_t read_u32();

  bool test_if_date_time_12b();

  bool parse_element(uint8_t type, uint8_t depth = 0);
  bool parse_sequence(uint8_t type, uint8_t depth = 0);
  bool parse_data(uint8_t type, uint8_t depth = 0);

  bool parse_attribute_descriptor_unsafe(bool tagged);
  bool parse_attribute_value(uint16_t class_id, const uint8_t *obis_code, uint8_t attribute_id);

  bool with_position_rollback(std::function<bool(AxdrParserFast *)> func);

  // Skip over data without parsing
  bool skip_data(uint8_t type);
  bool skip_sequence(uint8_t type);

 public:
  AxdrParserFast(gxByteBuffer *buf, AttributeDescriptorCallback callback) : buffer(buf), callback_(callback) {}
  size_t parse();
};