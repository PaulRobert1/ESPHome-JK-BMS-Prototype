#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <algorithm>
namespace esphome::jk_bms_uart {
// Fixed storage. Rescan on checksum failure, not on an embedded payload preamble.
class StreamParser {
 public:
  uint32_t bad_checksums{0};
  size_t size() const {return used_;}
  void reset(){used_=0;}
  template<class Callback> void feed(uint8_t byte, Callback cb) {
    buffer_[used_++]=byte;
    while(used_ && !prefix_()) discard_();
    if(used_<300) return;
    uint8_t sum=0;for(size_t i=0;i<299;i++)sum+=buffer_[i];
    if(sum==buffer_[299]) {cb(buffer_);used_=0;}
    else {bad_checksums++;discard_();while(used_ && !prefix_())discard_();}
  }
 private:
  std::array<uint8_t,300> buffer_{};size_t used_{0};
  bool prefix_() const {const uint8_t h[]={0x55,0xaa,0xeb,0x90};for(size_t i=0;i<std::min(used_,size_t(4));i++)if(buffer_[i]!=h[i])return false;return true;}
  void discard_(){std::move(buffer_.begin()+1,buffer_.begin()+used_,buffer_.begin());--used_;}
};
struct WriteGate {
 bool unlocked{false},pending{false};uint32_t unlocked_at{0};
 void unlock(uint32_t now){unlocked=true;unlocked_at=now;}
 void lock(){unlocked=false;}
 bool allowed(uint32_t now,bool fresh,bool identity) const {return unlocked && uint32_t(now-unlocked_at)<60000 && !pending && fresh && identity;}
};
}
