#include "veml6075.h"
#include "esphome/core/log.h"

namespace esphome {
namespace veml6075 {

static const char *const TAG = "veml6075.sensor";

void VEML6075Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up VEML6075");

  if (!this->configure()) {
    ESP_LOGE(TAG, "Sensor configuration failed");
    this->mark_failed();
  } else {
    state_ = State::WAITING_READY;

    // (Same as VEML7700 component -- wait 2x read_delays before reading)
    this->set_timeout(2 * this->_read_delay, [this]() { this->state_ = State::READY; });
  }
}

bool VEML6075Component::configure(void) {
  ESP_LOGV(TAG, "Starting to configure sensor");

  _command_register.reg = 0;

  uint8_t data[2] = {0x00};
  i2c::ErrorCode status = this->read_register(REG_ID, data, 2);
  if (status != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Failed to read manufacturer ID");
    return false;
  }

  if (data[0] != MANUFACTURER_ID) {
    ESP_LOGE(TAG, "Wrong manufacturer id, expected 0x%.2X but got 0x%.2X", MANUFACTURER_ID, data[0]);
    return false;
  }

  if (this->shutdown(true) != i2c::ERROR_OK) {
    return false;
  }

  if (this->setForcedMode(false) != i2c::ERROR_OK) {
    return false;
  }

  if (this->setIntegrationTime(_integration_time) != i2c::ERROR_OK) {
    return false;
  }

  if (this->setHighDynamic(_hd_enabled) != i2c::ERROR_OK) {
    return false;
  }

  if (this->shutdown(false) != i2c::ERROR_OK) {
    return false;
  }

  ESP_LOGV(TAG, "Sensor successfully configured");

  return true;
}

i2c::ErrorCode VEML6075Component::shutdown(bool sd) {
  _command_register.SD = sd;
  auto err = this->write_register(REG_UV_CONF, _command_register.reg_bytes, 2);
  if (err != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Shutdown command failed");
  }
  return err;
}

i2c::ErrorCode VEML6075Component::setForcedMode(bool flag) {
  _command_register.UV_AF = flag;
  auto err = this->write_register(REG_UV_CONF, _command_register.reg_bytes, 2);
  if (err != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Set forced mode failed");
  }
  return err;
}

i2c::ErrorCode VEML6075Component::setIntegrationTime(veml6075_integrationTime itime) {
  _integration_time = itime;
  // Set integration time
  _command_register.UV_IT = (uint8_t) _integration_time;
  auto err = this->write_register(REG_UV_CONF, _command_register.reg_bytes, 2);
  if (err != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Set integration time failed");
  }

  _read_delay = 0;
  switch (_integration_time) {
    case VEML6075_50MS:
      _read_delay = 50;
      break;
    case VEML6075_100MS:
      _read_delay = 100;
      break;
    case VEML6075_200MS:
      _read_delay = 200;
      break;
    case VEML6075_400MS:
      _read_delay = 400;
      break;
    case VEML6075_800MS:
      _read_delay = 800;
      break;
  }

  return err;
}

i2c::ErrorCode VEML6075Component::setHighDynamic(bool hd) {
  _hd_enabled = hd;

  _command_register.UV_HD = _hd_enabled;
  auto err = this->write_register(REG_UV_CONF, _command_register.reg_bytes, 2);
  if (err != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Set high dynamic mode failed");
  }
  return err;
}

i2c::ErrorCode VEML6075Component::takeReading(void) {
  uint8_t uva_data[2] = {0x00};
  uint8_t uvb_data[2] = {0x00};
  uint8_t uvcomp1_data[2] = {0x00};
  uint8_t uvcomp2_data[2] = {0x00};

  // Take the reading immediately
  auto err1 = this->read_register(REG_UVA_DATA, uva_data, 2);
  auto err2 = this->read_register(REG_UVB_DATA, uvb_data, 2);
  auto err3 = this->read_register(REG_UVCOMP1_DATA, uvcomp1_data, 2);
  auto err4 = this->read_register(REG_UVCOMP2_DATA, uvcomp2_data, 2);

  if (err1 != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Read UVA register failed");
    return err1;
  }
  if (err2 != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Read UVB register failed");
    return err2;
  }
  if (err3 != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Read UV Comp1 register failed");
    return err3;
  }
  if (err4 != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Read UV Comp2 register failed");
    return err4;
  }

  uint16_t rawUVA = (uva_data[0] & 0x00FF) | ((uva_data[1] & 0x00FF) << 8);
  uint16_t rawUVB = (uvb_data[0] & 0x00FF) | ((uvb_data[1] & 0x00FF) << 8);
  uint16_t comp1 = (uvcomp1_data[0] & 0x00FF) | ((uvcomp1_data[1] & 0x00FF) << 8);
  uint16_t comp2 = (uvcomp2_data[0] & 0x00FF) | ((uvcomp2_data[1] & 0x00FF) << 8);

  float uvaCalc =
      (float) rawUVA - ((UVA_A_COEF * UV_ALPHA * comp1) / UV_GAMMA) - ((UVA_B_COEF * UV_ALPHA * comp2) / UV_DELTA);

  float uvbCalc =
      (float) rawUVB - ((UVB_C_COEF * UV_BETA * comp1) / UV_GAMMA) - ((UVB_D_COEF * UV_BETA * comp2) / UV_DELTA);

  float uvia = uvaCalc * (1.0 / UV_ALPHA) * _aResponsivity;
  float uvib = uvbCalc * (1.0 / UV_BETA) * _bResponsivity;
  float uvIndex = (uvia + uvib) / 2.0;

  if (_hd_enabled) {
    uvIndex *= HD_SCALAR;
  }

  ESP_LOGV(TAG, "Read success with UVAcalc %f and UVBcalc %f ", uvaCalc, uvbCalc);
  ESP_LOGV(TAG, "UV index: %f ", uvIndex);

  // Publish
  if (this->uva_sensor_ != nullptr) {
    this->uva_sensor_->publish_state(uvaCalc);
  }
  if (this->uvb_sensor_ != nullptr) {
    this->uvb_sensor_->publish_state(uvbCalc);
  }
  if (this->uv_index_sensor_ != nullptr) {
    this->uv_index_sensor_->publish_state(uvIndex);
  }

  if (this->raw1_sensor_ != nullptr) {
    this->raw1_sensor_->publish_state(rawUVA);
  }
  if (this->raw2_sensor_ != nullptr) {
    this->raw2_sensor_->publish_state(rawUVB);
  }
  if (this->raw3_sensor_ != nullptr) {
    this->raw3_sensor_->publish_state(comp1);
  }
  if (this->raw4_sensor_ != nullptr) {
    this->raw4_sensor_->publish_state(comp2);
  }

  return i2c::ERROR_OK;
}

void VEML6075Component::dump_config() {
  if (state_ == State::READY) {
    ESP_LOGCONFIG(TAG, "Sensor started successfully!");
  } else if (state_ == State::INIT) {
    ESP_LOGCONFIG(TAG, "Sensor failed to initialise!");
  } else {
    ESP_LOGCONFIG(TAG, "Sensor in invalid state / not started!");
  }
  ESP_LOGCONFIG(TAG, "Integration time: %hu", _read_delay);
  ESP_LOGCONFIG(TAG, "High dynamic mode: %d", _command_register.UV_HD);
}

i2c::ErrorCode VEML6075Component::checkReconfigure(void) {
  // Read the config register
  uint8_t data[2] = {0x00};
  i2c::ErrorCode err = this->read_register(REG_UV_CONF, data, 2);
  if (err != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Failed to read config register while checking for reconfiguration");
    return err;
  }

  ConfigurationRegister current_cr = {};
  current_cr.reg_bytes[0] = data[0];
  current_cr.reg_bytes[1] = data[1];

  bool reconfigure = false;

  if (current_cr.UV_IT != (uint8_t) _integration_time) {
  // if (true) {
    // Reconfigure integration time
    ESP_LOGV(TAG, "Integration time changed, reconfiguring sensor");
    reconfigure = true;
  }

  if (current_cr.UV_HD != _hd_enabled) {
  // if (true) {
    // Reconfigure HD mode
    ESP_LOGV(TAG, "High dynamic mode changed, reconfiguring sensor");
    reconfigure = true;
  }

  if (!reconfigure) {
    // We're done
    return i2c::ERROR_OK;
  }

  if (!this->configure()) {
    return i2c::ERROR_UNKNOWN;
  }

  state_ = State::WAITING_READY;

  // Again wait 2x delays to take next reading
  this->set_timeout(2 * this->_read_delay, [this]() { this->state_ = State::READY; });

  return i2c::ERROR_OK;
}

void VEML6075Component::update() {
  // Check sensor needs reconfiguring
  if (state_ == State::READY && checkReconfigure() != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Failed to reconfigure sensor!");
    return;
  }
  

  if (state_ == State::READY) {
    // Take and publish reading
    if (takeReading() != i2c::ERROR_OK) {
      ESP_LOGE(TAG, "Failed to take reading - communication error!");
    }

    // Debug info
    uint8_t data[2] = {0x00};
    this->read_register(REG_ID, data, 2);
    ESP_LOGV(TAG, "Manufacturer id [0x%.2X, 0x%.2X]", data[0], data[1]);

    data[2] = {0x00};
    this->read_register(REG_UV_CONF, data, 2);
    ESP_LOGV(TAG, "Conf reg [0x%.2X, 0x%.2X]", data[0], data[1]);
  } else {
    ESP_LOGI(TAG, "Sensor not initialised, failed to take reading");
  }
}

}  // namespace veml6075
}  // namespace esphome
