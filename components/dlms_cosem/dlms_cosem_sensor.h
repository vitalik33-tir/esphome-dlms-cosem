#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace dlms_cosem {

// базовый класс
class DlmsCosemSensorBase {
 public:
  virtual void publish() = 0;
  virtual const std::string &get_obis_code() const = 0;
  virtual ~DlmsCosemSensorBase() = default;
};

// числовой сенсор (kWh, W, V, A и т.д.)
class DlmsCosemSensor : public DlmsCosemSensorBase, public sensor::Sensor {
 public:
  explicit DlmsCosemSensor(std::string obis_code, uint8_t attribute, uint8_t request_retries)
      : obis_code_(std::move(obis_code)),
        attribute_(attribute),
        request_retries_(request_retries) {}

  const std::string &get_obis_code() const override { return obis_code_; }

  void publish() override {
    if (has_value_) {
      publish_state(value_);
      has_value_ = false;
    }
  }

  void set_value(float value) {
    value_ = value;
    has_value_ = true;
  }

 protected:
  std::string obis_code_;
  uint8_t attribute_;
  uint8_t request_retries_;
  float value_{0};
  bool has_value_{false};
};

}  // namespace dlms_cosem
}  // namespace esphome
