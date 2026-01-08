#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

namespace esphome {
namespace dlms_cosem {

class DlmsCosemSensorBase {
 public:
  static const uint8_t MAX_REQUEST_TRIES = 5;

  explicit DlmsCosemSensorBase(std::string obis_code, uint8_t attribute, uint8_t request_retries)
      : obis_code_(std::move(obis_code)), attribute_(attribute), tries_(request_retries), max_tries_(request_retries) {}

  const std::string &get_obis_code() const { return this->obis_code_; }

  bool is_energy_obis_() const {
    // OBIS format: A.B.C.D.E.F ; D=8 denotes cumulative energy registers.
    const std::string &s = this->obis_code_;
    int field = 0;
    size_t start = 0;
    for (size_t i = 0; i <= s.size(); i++) {
      if (i == s.size() || s[i] == '.') {
        if (field == 3) {
          int d = 0;
          for (size_t k = start; k < i; k++) {
            char c = s[k];
            if (c < '0' || c > '9') return false;
            d = d * 10 + (c - '0');
          }
          return d == 8;
        }
        field++;
        start = i + 1;
      }
    }
    return false;
  }

  uint8_t get_attribute() const { return this->attribute_; }

  uint8_t get_tries_left() const { return this->tries_; }

  uint8_t get_max_tries() const { return this->max_tries_; }

  bool should_retry() const { return this->tries_ > 0; }

  void request_failed() {
    if (this->tries_ > 0) {
      this->tries_--;
    }
  }

  virtual void reset() {
    this->tries_ = this->max_tries_;
    this->has_value_ = false;
  }

  virtual bool has_value() const { return this->has_value_; }

  virtual void publish() = 0;

 protected:
  std::string obis_code_;
  uint8_t attribute_;
  bool has_value_{false};
  uint8_t tries_;
  uint8_t max_tries_;
};

class DlmsCosemSensor : public DlmsCosemSensorBase {
 public:
  explicit DlmsCosemSensor(std::string obis_code, uint8_t attribute, uint8_t request_retries)
      : DlmsCosemSensorBase(std::move(obis_code), attribute, request_retries) {}

  virtual const std::string &get_id() const = 0;
};

class DlmsCosemNumericSensor : public DlmsCosemSensor, public sensor::Sensor {
 public:
  explicit DlmsCosemNumericSensor(std::string obis_code, uint8_t attribute, uint8_t request_retries)
      : DlmsCosemSensor(std::move(obis_code), attribute, request_retries) {}

  void set_scale(float scale) { this->scale_f_ = scale; }

  void set_multiplier(float multiplier) { this->multiplier_ = multiplier; }

  void set_value(float value) {
    // Some meters report energy counters (OBIS *.8.*) with a negative sign.
    if (value < 0.0f && this->is_energy_obis_()) {
      value = -value;
    }
    this->value_ = value * scale_f_ * multiplier_;
    this->has_value_ = true;
    this->tries_ = 0;
  }

  void publish() override {
    if (this->has_value_) {
      this->publish_state(this->value_);
    }
  }

  const std::string &get_id() const override { return this->object_id; }

 protected:
  float scale_f_{1.0f};
  float multiplier_{1.0f};
  float value_{0.0f};
};

class DlmsCosemTextSensor : public DlmsCosemSensor, public text_sensor::TextSensor {
 public:
  explicit DlmsCosemTextSensor(std::string obis_code, uint8_t attribute, uint8_t request_retries)
      : DlmsCosemSensor(std::move(obis_code), attribute, request_retries) {}

  void set_value(std::string value) {
    this->value_ = std::move(value);
    this->has_value_ = true;
    this->tries_ = 0;
  }

  void publish() override {
    if (this->has_value_) {
      this->publish_state(this->value_);
    }
  }

  const std::string &get_id() const override { return this->object_id; }

 protected:
  std::string value_;
};

class DlmsCosemBinarySensor : public DlmsCosemSensor, public binary_sensor::BinarySensor {
 public:
  explicit DlmsCosemBinarySensor(std::string obis_code, uint8_t attribute, uint8_t request_retries)
      : DlmsCosemSensor(std::move(obis_code), attribute, request_retries) {}

  void set_value(bool value) {
    this->value_ = value;
    this->has_value_ = true;
    this->tries_ = 0;
  }

  void publish() override {
    if (this->has_value_) {
      this->publish_state(this->value_);
    }
  }

  const std::string &get_id() const override { return this->object_id; }

 protected:
  bool value_{false};
};

}  // namespace dlms_cosem
}  // namespace esphome
