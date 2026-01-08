#pragma once

#include <string>
#include <utility>

#include "esphome/core/entity_base.h"

#include "esphome/components/sensor/sensor.h"

#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif

#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif

namespace esphome {
namespace dlms_cosem {

/**
 * We keep a light abstraction over different entity types that can be bound to an OBIS code.
 * The python codegen creates objects with the default ctor and then calls setters
 * (set_name_and_object_id, set_obis_code, set_attribute, set_request_retries, ...).
 */
enum class SensorType : uint8_t {
  SENSOR = 0,
  TEXT_SENSOR = 1,
  BINARY_SENSOR = 2,
};

class DlmsCosemSensorBase {
 public:
  DlmsCosemSensorBase() = default;
  virtual ~DlmsCosemSensorBase() = default;

  // Identification
  void set_name_and_object_id(const std::string &name, const std::string &object_id) {
    this->sensor_name_ = name;
    this->object_id_ = object_id;
  }
  const std::string &get_sensor_name() const { return this->sensor_name_; }
  virtual const std::string &get_id() const = 0;

  // OBIS/COSEM parameters
  void set_obis_code(std::string obis_code) { this->obis_code_ = std::move(obis_code); }
  const std::string &get_obis_code() const { return this->obis_code_; }

  void set_obis_class(uint16_t obis_class) { this->obis_class_ = obis_class; }
  uint16_t get_obis_class() const { return this->obis_class_; }

  void set_attribute(uint8_t attribute) { this->attribute_ = attribute; }
  uint8_t get_attribute() const { return this->attribute_; }

  void set_request_retries(uint8_t request_retries) { this->request_retries_ = request_retries; }
  uint8_t get_request_retries() const { return this->request_retries_; }

  // Publish control
  void set_dont_publish(bool dont_publish) { this->dont_publish_ = dont_publish; }
  bool shall_we_publish() const { return !this->dont_publish_; }

  SensorType get_type() const { return this->type_; }

  // Scale/unit support (numeric sensors); other types just report "ready".
  virtual bool has_got_scale_and_unit() const { return true; }

  // Access to underlying entity (for register + name reads)
  virtual EntityBase *get_base() = 0;

  // Called by component when new value arrived
  virtual void publish() = 0;

 protected:
  SensorType type_{SensorType::SENSOR};

  std::string sensor_name_{};
  std::string object_id_{};

  std::string obis_code_{};
  uint16_t obis_class_{0};
  uint8_t attribute_{2};
  uint8_t request_retries_{3};

  bool dont_publish_{false};
};

// Numeric sensor (sensor::Sensor)
class DlmsCosemSensor : public DlmsCosemSensorBase, public sensor::Sensor {
 public:
  DlmsCosemSensor() { this->type_ = SensorType::SENSOR; }

  // setters used by codegen / component
  void set_multiplier(float multiplier) { this->multiplier_ = multiplier; }

  bool has_got_scale_and_unit() const override { return this->has_scale_and_unit_; }
  void set_scale_and_unit(int scal, int unit, std::string unit_str) {
    this->scale_ = scal;
    this->unit_ = unit;
    this->unit_str_ = std::move(unit_str);
    this->has_scale_and_unit_ = true;
  }
  int get_scale() const { return this->scale_; }
  int get_unit() const { return this->unit_; }
  const std::string &get_unit_str() const { return this->unit_str_; }

  // value set by component
  void set_value(float value) {
    this->value_ = value * this->multiplier_;
    this->has_value_ = true;
    this->tries_ = 0;
  }

  void publish() override {
    if (!this->shall_we_publish())
      return;
    if (this->has_value_) {
      this->publish_state(this->value_);
      this->has_value_ = false;
    }
  }

  const std::string &get_id() const override { return this->object_id_; }

  EntityBase *get_base() override { return this; }

 protected:
  float value_{NAN};
  bool has_value_{false};
  uint8_t tries_{0};

  float multiplier_{1.0f};

  int scale_{0};
  int unit_{0};
  std::string unit_str_{};
  bool has_scale_and_unit_{false};
};

#ifdef USE_TEXT_SENSOR
class DlmsCosemTextSensor : public DlmsCosemSensorBase, public text_sensor::TextSensor {
 public:
  DlmsCosemTextSensor() { this->type_ = SensorType::TEXT_SENSOR; }

  explicit DlmsCosemTextSensor(std::string obis_code, uint8_t attribute, uint8_t request_retries) {
    this->type_ = SensorType::TEXT_SENSOR;
    this->obis_code_ = std::move(obis_code);
    this->attribute_ = attribute;
    this->request_retries_ = request_retries;
  }

  void set_value(const char *value, bool cp1251_conversion_required) {
    std::string s(value ? value : "");
    if (cp1251_conversion_required) {
      // Best-effort: ESPHome strings are UTF-8; if device returns CP1251 bytes and user enabled conversion,
      // we just keep bytes as-is here. Proper conversion requires a mapping table; keep behavior non-crashing.
      // If you need strict CP1251->UTF8, implement it here.
    }
    this->value_ = std::move(s);
    this->has_value_ = true;
    this->tries_ = 0;
  }

  void publish() override {
    if (!this->shall_we_publish())
      return;
    if (this->has_value_) {
      this->publish_state(this->value_);
      this->has_value_ = false;
    }
  }

  const std::string &get_id() const override { return this->object_id_; }

  EntityBase *get_base() override { return this; }

 protected:
  std::string value_{};
  bool has_value_{false};
  uint8_t tries_{0};
};
#endif  // USE_TEXT_SENSOR

#ifdef USE_BINARY_SENSOR
class DlmsCosemBinarySensor : public DlmsCosemSensorBase, public binary_sensor::BinarySensor {
 public:
  DlmsCosemBinarySensor() { this->type_ = SensorType::BINARY_SENSOR; }

  explicit DlmsCosemBinarySensor(std::string obis_code, uint8_t attribute, uint8_t request_retries) {
    this->type_ = SensorType::BINARY_SENSOR;
    this->obis_code_ = std::move(obis_code);
    this->attribute_ = attribute;
    this->request_retries_ = request_retries;
  }

  void set_value(bool value) {
    this->value_ = value;
    this->has_value_ = true;
    this->tries_ = 0;
  }

  void publish() override {
    if (!this->shall_we_publish())
      return;
    if (this->has_value_) {
      this->publish_state(this->value_);
      this->has_value_ = false;
    }
  }

  const std::string &get_id() const override { return this->object_id_; }

  EntityBase *get_base() override { return this; }

 protected:
  bool value_{false};
  bool has_value_{false};
  uint8_t tries_{0};
};
#endif  // USE_BINARY_SENSOR

}  // namespace dlms_cosem
}  // namespace esphome
