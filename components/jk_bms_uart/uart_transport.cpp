#include "jk_bms_uart.h"
#include "esphome/core/log.h"
#include <cmath>
#include <cstdlib>
namespace esphome::jk_bms_uart {
static const char *const UART_TAG="jk_bms_uart.transport";
void JkBmsUart::set_transport_status_(const std::string &s) {if(transport_status_)transport_status_->publish_state(s);}
void JkBmsUart::set_control_status_(const std::string &s) {if(control_status_)control_status_->publish_state(s);ESP_LOGI(UART_TAG,"%s",s.c_str());}
void JkBmsUart::setup() {
 for(auto *p:{stat_pin_,aled_pin_})if(p){p->setup();p->digital_write(false);}
 for(auto *p:{reset_pin_,pwrc_pin_})if(p)p->setup();
 at_line_.reserve(96);
 set_transport_status_("Passive - UART RX only; session disabled");
 set_control_status_("Locked");
}
void JkBmsUart::invalidate_link_() {
 identity_ok_=false;protocol_known_=false;have_frame_=false;have_settings_=false;gate_.lock();parser_.reset();at_line_.clear();
 if(gate_.pending){gate_.pending=false;pending_number_=nullptr;pending_switch_=nullptr;set_control_status_("Outcome unknown: link interrupted; no automatic retry");}
 publish_device_unavailable_();
}
void JkBmsUart::set_session(bool enabled) {
 session_=enabled;session_since_=millis();invalidate_link_();stat_asserted_=false;
 for(auto *p:{stat_pin_,aled_pin_})if(p)p->digital_write(false);
 set_transport_status_(enabled?"Starting UART session":"Passive - UART RX only; session disabled");
}
void JkBmsUart::unlock_controls(bool enabled) {
 if(!enabled){gate_.lock();set_control_status_(gate_.pending?"Locked; awaiting pending result":"Locked");return;}
 if(!session_ || !identity_ok_ || !link_online()){gate_.lock();set_control_status_("Unlock refused: verified model and live telemetry required");return;}
 gate_.unlock(millis());set_control_status_("Unlocked for 60 seconds; one change at a time");
}
void JkBmsUart::loop() {
 const uint32_t now=millis();
 bool reset=reset_pin_ && !reset_pin_->digital_read();
 if(reset && !reset_asserted_){invalidate_link_();stat_asserted_=false;for(auto *p:{stat_pin_,aled_pin_})if(p)p->digital_write(false);set_transport_status_("BMS asserted module RESET");}
 if(!reset && reset_asserted_){session_since_=now;set_transport_status_("Module RESET released; restarting handshake");}
 reset_asserted_=reset;
 if(session_ && !reset && !stat_asserted_ && uint32_t(now-session_since_)>=2000){
  // Emulates a client connection. This is a testable hypothesis for this BMS revision.
  if(stat_pin_) stat_pin_->digital_write(true);
  if(aled_pin_) aled_pin_->digital_write(true);
  stat_asserted_=true;
 }
 if(parser_.size() && uint32_t(now-last_rx_)>250)parser_.reset();
 size_t budget=512;uint8_t b;
 while(budget-- && available() && read_byte(&b)){
  last_rx_=now;rx_bytes_++;
  bool binary=parser_.size()!=0 || b==0x55;
  if(!binary)feed_at_(b);else at_line_.clear();
  parser_.feed(b,[this](const auto &f){accept_frame_(f);});
 }
 if(have_frame_ && !link_online()){invalidate_link_();set_transport_status_("Offline: no valid frame for 15 seconds");}
 if(gate_.unlocked && !controls_unlocked()){gate_.lock();set_control_status_(gate_.pending?"Auto-locked; awaiting pending result":"Auto-locked after 60 seconds");}
 if(gate_.pending && uint32_t(now-pending_since_)>10000){
  gate_.pending=false;pending_number_=nullptr;pending_switch_=nullptr;gate_.lock();
  set_control_status_("Not verified after 10 seconds; outcome unknown, check reported value");
 }
}
void JkBmsUart::feed_at_(uint8_t b) {
 if(!session_ || reset_asserted_ || !at_emulation_)return;
 if(b=='\n'){
  if(!at_line_.empty() && at_line_.back()=='\r')at_line_.pop_back();
  // Only documented/observed queries. Never blanket-ack unknown configuration commands.
  std::string reply;
  if(at_line_=="AT")reply="OK\r\n";
  else if(at_line_=="AT+VERSION")reply="+VER=JDY-18-V1.91\r\n";
  else if(at_line_=="AT+BAUD")reply="+BAUD=8\r\n";
  else if(at_line_=="AT+STAT")reply=stat_asserted_?"+STAT=1\r\n":"+STAT=0\r\n";
  if(!reply.empty()){write_str(reply.c_str());last_at_=millis();ESP_LOGD(UART_TAG,"Responded to module identity/status query");}
  else if(at_line_.rfind("AT",0)==0){ESP_LOGW(UART_TAG,"Unsupported module AT command (%u bytes); not acknowledged",unsigned(at_line_.size()));set_transport_status_("Unsupported AT command: capture original handshake");}
  at_line_.clear();return;
 }
 if((b>=32 && b<=126)||b=='\r'){if(at_line_.size()<95)at_line_+=char(b);else at_line_.clear();}else at_line_.clear();
}
void JkBmsUart::update() {
 if(!session_ || reset_asserted_ || !stat_asserted_ || (pwrc_pin_ && !pwrc_pin_->digital_read()))return;
 if(uint32_t(millis()-last_at_)<500)return;
 if(!protocol_known_)write_register(0x97,0,0);
 else write_register(0x96,0,0);
}
void JkBmsUart::accept_frame_(const std::array<uint8_t,300> &f) {
 if(reset_asserted_ || (f[4]!=1 && f[4]!=2 && f[4]!=3))return;
 valid_frames_++;
 if(f[4]==3){
  auto field=[&](size_t a,size_t n){std::string s;for(size_t i=a;i<a+n && f[i];i++){if(f[i]<32||f[i]>126)return std::string();s+=char(f[i]);}return s;};
  std::string model=field(6,16),hardware=field(22,8);
  for(char &c:model)if(c=='_')c='-';
  const char *h=hardware.c_str();if(*h=='V'||*h=='v')h++;
  int major=std::atoi(h);
  protocol_known_=model.rfind("JK-",0)==0 && major>=6 && major<=19;
  if(protocol_known_)protocol_version_=major>=11?PROTOCOL_VERSION_JK02_32S:PROTOCOL_VERSION_JK02_24S;
  identity_ok_=protocol_known_ && model==expected_model_;
  if(!identity_ok_) {
    gate_.lock();
    if(gate_.pending){gate_.pending=false;pending_number_=nullptr;pending_switch_=nullptr;set_control_status_("Outcome unknown: device identity mismatch");}
  }
  ESP_LOGI(UART_TAG,"Reported model=%s hardware=%s; controls identity %s",model.c_str(),hardware.c_str(),identity_ok_?"matched":"BLOCKED");
  set_transport_status_(protocol_known_?(identity_ok_?"Identity matched; waiting for telemetry":"Model mismatch: telemetry only; controls blocked"):"Unknown hardware protocol: controls blocked");
 }
 if(!protocol_known_ && f[4]!=3)return;
 if(f[4]==1){have_settings_=true;last_settings_=millis();}
 std::vector<uint8_t> data(f.begin(),f.end());decode_(data);
 if(f[4]==2){have_frame_=true;last_valid_=millis();set_transport_status_(identity_ok_?"Online: JK UART telemetry":"Online: model mismatch, controls blocked");}
}
bool JkBmsUart::write_register(uint8_t address,uint32_t value,uint8_t length) {
 // Public legacy entry point accepts only read requests. Controls use typed request methods.
 if(!session_ || reset_asserted_ || !stat_asserted_ || (pwrc_pin_ && !pwrc_pin_->digital_read()))return false;
 if(length!=0 || value!=0 || (address!=0x96 && address!=0x97))return false;
 auto frame=build_frame(address,value,length);write_array(frame.data(),frame.size());return true;
}
void JkBmsUart::request_number(number::Number *obj,uint8_t address,uint32_t value,uint8_t length,float expected) {
 // First-prototype controls: documented JK02 registers only. No raw writes, calibration, reset or protection bypass.
 bool allowed=address==0x02||address==0x03||address==0x04||address==0x05||address==0x06||address==0x0c||address==0x0f||address==0x13||address==0x20||(protocol_version_==PROTOCOL_VERSION_JK02_24S?address==0x26:address==0x22);
 if(!obj || !std::isfinite(expected) || !allowed || length!=4 || !session_ || reset_asserted_ || (pwrc_pin_&&!pwrc_pin_->digital_read()) || !gate_.allowed(millis(),link_online() && have_settings_ && uint32_t(millis()-last_settings_)<15000,identity_ok_)){
  set_control_status_("Change refused: locked, busy, offline, unsupported or model mismatch");return;
 }
 if(obj->has_state() && std::fabs(obj->state-expected)<0.0005f){set_control_status_("Already at requested value; nothing sent");return;}
 pending_number_=obj;pending_switch_=nullptr;pending_expected_=expected;pending_since_=millis();gate_.pending=true;
 auto frame=build_frame(address,value,length);write_array(frame.data(),frame.size());
 set_control_status_("Sent once; awaiting reported value (10 seconds)");
 // Periodic 0x96 reads verify the result. Never resend a setting automatically.
}
void JkBmsUart::request_switch(switch_::Switch *obj,uint8_t address,uint32_t value,uint8_t length,bool expected) {
 if(!obj || address<0x1d || address>0x1f || length!=4 || !session_ || reset_asserted_ || (pwrc_pin_&&!pwrc_pin_->digital_read()) || !gate_.allowed(millis(),link_online() && have_settings_ && uint32_t(millis()-last_settings_)<15000,identity_ok_)){
  set_control_status_("Change refused: locked, busy, offline, unsupported or model mismatch");return;
 }
 pending_switch_=obj;pending_number_=nullptr;pending_expected_=expected?1.0f:0.0f;pending_since_=millis();gate_.pending=true;
 auto frame=build_frame(address,value,length);write_array(frame.data(),frame.size());set_control_status_("Sent once; awaiting reported switch state (10 seconds)");
}
void JkBmsUart::finish_pending_(float actual) {
 if(!gate_.pending || !std::isfinite(actual) || std::fabs(actual-pending_expected_)>0.0005f)return;
 gate_.pending=false;pending_number_=nullptr;pending_switch_=nullptr;set_control_status_("Verified: BMS reports requested value");
}
}
