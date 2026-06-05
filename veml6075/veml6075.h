#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace veml6075 {

union ConfigurationRegister {
  uint16_t reg;
  uint8_t reg_bytes[2];
  struct {
    uint8_t SD : 1;           ///< Shut Down
    uint8_t UV_AF : 1;        ///< Auto or forced
    uint8_t UV_TRIG : 1;      ///< Trigger forced mode
    uint8_t UV_HD : 1;        ///< High dynamic
    uint8_t UV_IT : 3;        ///< Integration Time
    uint8_t high_byte;        ///< unused
  } __attribute__((packed));  ///< Bitfield of 16 bits
};

typedef enum {
  REG_UV_CONF = 0x00,
  REG_UVA_DATA = 0x07,
  REG_UVB_DATA = 0x09,
  REG_UVCOMP1_DATA = 0x0A,
  REG_UVCOMP2_DATA = 0x0B,
  REG_ID = 0x0C
} veml6075_register;

typedef enum {
  VEML6075_50MS = 0b000,
  VEML6075_100MS = 0b001,
  VEML6075_200MS = 0b010,
  VEML6075_400MS = 0b011,
  VEML6075_800MS = 0b100
} veml6075_integrationTime;

static const uint8_t MANUFACTURER_ID = 0x26;

class VEML6075Component : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  void update() override;

  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_uva_sensor(sensor::Sensor *sensor) { this->uva_sensor_ = sensor; }
  void set_uvb_sensor(sensor::Sensor *sensor) { this->uvb_sensor_ = sensor; }
  void set_uv_index_sensor(sensor::Sensor *sensor) { this->uv_index_sensor_ = sensor; }

  void set_raw1_sens(sensor::Sensor *sensor) { this->raw1_sensor_ = sensor; }
  void set_raw2_sens(sensor::Sensor *sensor) { this->raw2_sensor_ = sensor; }
  void set_raw3_sens(sensor::Sensor *sensor) { this->raw3_sensor_ = sensor; }
  void set_raw4_sens(sensor::Sensor *sensor) { this->raw4_sensor_ = sensor; }

  void set_integration_time(veml6075_integrationTime itime) { _integration_time = itime; }
  void set_high_dynamic(bool hd) { _hd_enabled = hd; }

 protected:
  enum class State : uint8_t {
    INIT,
    WAITING_READY,  // Set when the sensor has just been started to allow time for readings to accumulate
    READY,
  } state_{State::INIT};

  // Configuration variables
  veml6075_integrationTime _integration_time = VEML6075_100MS;
  bool _hd_enabled = false;

  ConfigurationRegister _command_register;
  uint16_t _read_delay;

  // Sensor variables
  sensor::Sensor *uva_sensor_{nullptr};
  sensor::Sensor *uvb_sensor_{nullptr};
  sensor::Sensor *uv_index_sensor_{nullptr};
  sensor::Sensor *raw1_sensor_{nullptr};
  sensor::Sensor *raw2_sensor_{nullptr};
  sensor::Sensor *raw3_sensor_{nullptr};
  sensor::Sensor *raw4_sensor_{nullptr};

  bool configure(void);
  i2c::ErrorCode checkReconfigure(void);

  i2c::ErrorCode shutdown(bool sd);
  i2c::ErrorCode setForcedMode(bool flag);
  i2c::ErrorCode setIntegrationTime(veml6075_integrationTime itime);
  i2c::ErrorCode setHighDynamic(bool hd);
  i2c::ErrorCode takeReading(void);

  float UVA_A_COEF = 2.22;
  float UVA_B_COEF = 1.33;
  float UVB_C_COEF = 2.95;
  float UVB_D_COEF = 1.74;

  float _aResponsivity = 0.001461;  // should this change based on integration time
  float _bResponsivity = 0.002591;

  float UV_ALPHA = 1.0;
  float UV_BETA = 1.0;
  float UV_GAMMA = 1.0;
  float UV_DELTA = 1.0;

  // (From sparkfun code) Scalar to multiply UV Index by when High Dynamic enabled
  float HD_SCALAR = 2.0;
};

}  // namespace veml6075
}  // namespace esphome
