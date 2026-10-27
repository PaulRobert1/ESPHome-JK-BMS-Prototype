#include "jk_number.h"
#include "esphome/core/log.h"
#include <cstring>

// Fallback for ESPHome < 2026.10.0
#ifndef ESPHOME_LOG_TAG
#define ESPHOME_LOG_TAG(name, tag) static const char *const name = tag
#endif

namespace esphome::jk_bms_uart {

ESPHOME_LOG_TAG(TAG, "jk_bms_uart.number");

void JkNumber::dump_config() { LOG_NUMBER("", "JkBmsUart Number", this); }

void JkNumber::control(float value) {
  if (this->parent_->get_protocol_version() == PROTOCOL_VERSION_JK04 && this->jk04_holding_register_) {
    uint32_t payload = JkBmsUart::encode_jk04_payload(value, this->jk04_length_, this->factor_);
    this->parent_->request_number(this, this->jk04_holding_register_, payload, this->jk04_length_, value);
    return;
  }

  if (this->parent_->get_protocol_version() == PROTOCOL_VERSION_JK02_24S && this->jk02_holding_register_) {
    uint32_t payload = (uint32_t) (int32_t) (value * this->factor_);
    this->parent_->request_number(this, this->jk02_holding_register_, payload, this->length_, value);
    return;
  }

  if (this->parent_->get_protocol_version() == PROTOCOL_VERSION_JK02_32S && this->jk02_32s_holding_register_) {
    uint32_t payload = (uint32_t) (int32_t) (value * this->factor_);
    this->parent_->request_number(this, this->jk02_32s_holding_register_, payload, this->length_, value);
    return;
  }

  ESP_LOGE(TAG, "This number entity isn't supported by the selected protocol version");
}

}  // namespace esphome::jk_bms_uart
