#include "jk_switch.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"

// Fallback for ESPHome < 2026.10.0
#ifndef ESPHOME_LOG_TAG
#define ESPHOME_LOG_TAG(name, tag) static const char *const name = tag
#endif

namespace esphome::jk_bms_uart {

ESPHOME_LOG_TAG(TAG, "jk_bms_uart.switch");

void JkSwitch::dump_config() { LOG_SWITCH("", "JkBmsUart Switch", this); }
void JkSwitch::write_state(bool state) {
  if (this->parent_->get_protocol_version() == PROTOCOL_VERSION_JK04 && this->jk04_holding_register_) {
    this->parent_->request_switch(this, this->jk04_holding_register_, (state) ? 0x00000001 : 0x00000000, 0x01, state);
    return;
  }

  if (this->parent_->get_protocol_version() == PROTOCOL_VERSION_JK02_24S && this->jk02_holding_register_) {
    this->parent_->request_switch(this, this->jk02_holding_register_, (state) ? 0x00000001 : 0x00000000, 0x04, state);
    return;
  }

  if (this->parent_->get_protocol_version() == PROTOCOL_VERSION_JK02_32S && this->jk02_32s_holding_register_) {
    this->parent_->request_switch(this, this->jk02_32s_holding_register_, (state) ? 0x00000001 : 0x00000000, 0x04, state);
    return;
  }

  ESP_LOGE(TAG, "This switch isn't supported by the selected protocol version");
}

}  // namespace esphome::jk_bms_uart
