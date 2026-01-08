#pragma once

#include <string>
#include <utility>

#include "esphome/core/component.h"

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace dlms_cosem {

enum class SensorType : uint8_t { SENSOR = 0, TEXT_SENSOR = 1, BINARY_SENSOR = 2 };

class DlmsCosemSensorBase {
 public:
  virtual ~DlmsCosemSensorBase() = default;

  const std::string &get_obis_code() const { return this->obis_code_; }
  uint8_t get_attribute() const { return this->attribute_; }
  uint8_t get_request_retries() const { return this->request_retries_; }

  bool shall_we_publish() const { return !this->dont_publish_; }

  void set_obis_code(std::string obis_code) { this->obis_code_ = std::move(obis_code); }
  void set_attribute(uint8_t attribute) { this->attribute_ = attribute; }
  void set_request_retries(uint8_t request_retries) { this->request_retries_ = request_retries; }

  void set_dont_publish(bool dont_publish) { this->dont_publish_ = dont_publish; }

  uint8_t get_obis_class() const { return this->obis_class_; }
  void set_obis_class(uint8_t cls) { this->obis_class_ = cls; }

  SensorType get_type() const { return this->type_; }
  const std::string &get_sensor_name() const { return this->sensor_name_; }

  virtual void publish() = 0;
  virtual const std::string &get_id() const = 0;

 protected:
  DlmsCosemSensorBase(std::string obis_code, uint8_t attribute, uint8_t request_retries, SensorType type)
      : obis_code_(std::move(obis_code)), attribute_(attribute), request_retries_(request_retries), type_(type) {}

  std::string obis_code_;
  uint8_t attribute_{0};
  uint8_t request_retries_{0};
  bool dont_publish_{false};

  uint8_t obis_class_{0};

  SensorType type_{SensorType::SENSOR};
  std::string sensor_name_;

  std::string object_id_;
};

class DlmsCosemSensor : public DlmsCosemSensorBase, public sensor::Sensor {
 public:
  explicit DlmsCosemSensor(std::string obis_code, uint8_t attribute, uint8_t request_retries)
      : DlmsCosemSensorBase(std::move(obis_code), attribute, request_retries, SensorType::SENSOR) {}

  void set_name_and_object_id(const std::string &name, const std::string &object_id) {
    this->set_name(name);
    this->sensor_name_ = name;
    this->object_id_ = object_id;
  }

  void set_value(float value) {
    this->value_ = value;
    this->has_value_ = true;
    this->tries_ = 0;
  }

  void set_scale_and_unit(int scale, int unit, std::string unit_s) {
    this->scale_ = scale;
    this->unit_ = unit;
    this->unit_str_ = std::move(unit_s);
    this->got_scale_and_unit_ = true;
  }

  bool has_got_scale_and_unit() const { return this->got_scale_and_unit_; }
  int get_scale() const { return this->scale_; }
  int get_unit() const { return this->unit_; }
  const std::string &get_unit_str() const { return this->unit_str_; }

  void publish() override {
    if (this->has_value_) {
      this->publish_state(this->value_);
      this->has_value_ = false;
    }
  }

  const std::string &get_id() const override { return this->object_id_; }

 protected:
  float value_{0.0f};
  bool has_value_{false};
  uint8_t tries_{0};

  bool got_scale_and_unit_{false};
  int scale_{0};
  int unit_{0};
  std::string unit_str_;
};

class DlmsCosemNumericSensor : public DlmsCosemSensor {
 public:
  explicit DlmsCosemNumericSensor(std::string obis_code, uint8_t attribute, uint8_t request_retries)
      : DlmsCosemSensor(std::move(obis_code), attribute, request_retries) {
    this->type_ = SensorType::SENSOR;
  }
};

class DlmsCosemTextSensor : public DlmsCosemSensorBase, public text_sensor::TextSensor {
 public:
  explicit DlmsCosemTextSensor(std::string obis_code, uint8_t attribute, uint8_t request_retries)
      : DlmsCosemSensorBase(std::move(obis_code), attribute, request_retries, SensorType::TEXT_SENSOR) {}

  void set_name_and_object_id(const std::string &name, const std::string &object_id) {
    this->set_name(name);
    this->sensor_name_ = name;
    this->object_id_ = object_id;
  }

  void set_value(const char *value, bool cp1251_conversion_required = false) {
    std::string s(value);
    if (cp1251_conversion_required) {
      // conversion is handled elsewhere; keep as-is here
    }
    this->value_ = std::move(s);
    this->has_value_ = true;
    this->tries_ = 0;
  }

  void publish() override {
    if (this->has_value_) {
      this->publish_state(this->value_);
      this->has_value_ = false;
    }
  }

  const std::string &get_id() const override { return this->object_id_; }

 protected:
  std::string value_;
  bool has_value_{false};
  uint8_t tries_{0};
};

class DlmsCosemBinarySensor : public DlmsCosemSensorBase, public binary_sensor::BinarySensor {
 public:
  explicit DlmsCosemBinarySensor(std::string obis_code, uint8_t attribute, uint8_t request_retries)
      : DlmsCosemSensorBase(std::move(obis_code), attribute, request_retries, SensorType::BINARY_SENSOR) {}

  void set_name_and_object_id(const std::string &name, const std::string &object_id) {
    this->set_name(name);
    this->sensor_name_ = name;
    this->object_id_ = object_id;
  }

  void set_value(bool value) {
    this->value_ = value;
    this->has_value_ = true;
    this->tries_ = 0;
  }

  void publish() override {
    if (this->has_value_) {
      this->publish_state(this->value_);
      this->has_value_ = false;
    }
  }

  const std::string &get_id() const override { return this->object_id_; }

 protected:
  bool value_{false};
  bool has_value_{false};
  uint8_t tries_{0};
};

}  // namespace dlms_cosem
}  // namespace esphome
