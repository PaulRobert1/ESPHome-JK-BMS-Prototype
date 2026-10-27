#pragma once

#include "../jk_bms_uart.h"
#include "esphome/core/component.h"
#include "esphome/components/button/button.h"

namespace esphome::jk_bms_uart {

class JkBmsUart;
class JkButton : public button::Button, public Component {
 public:
  void set_parent(JkBmsUart *parent) { this->parent_ = parent; };
  void set_holding_register(uint8_t holding_register) { this->holding_register_ = holding_register; };
  void dump_config() override;
  void loop() override {}
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void press_action() override;
  JkBmsUart *parent_;
  uint8_t holding_register_{0};
};

}  // namespace esphome::jk_bms_uart
