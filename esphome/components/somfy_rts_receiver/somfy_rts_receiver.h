#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/somfy_rts_protocol/somfy_rts_protocol.h"

namespace esphome {
namespace somfy_rts_receiver {

// One decoded Somfy RTS frame, exposed to YAML automations via on_frame.
struct SomfyRtsFrame {
  somfy_rts::Command command;
  uint16_t rolling_code;
  uint32_t remote_code;
};

// Listens on a remote_receiver, runs the shared Somfy RTS decoder on every
// captured burst, and logs / reports the first frame of each press. Repeat
// frames of the same press (same remote + rolling code + command, within
// dedup_window) are collapsed so a single button press is reported once.
class SomfyRtsReceiver : public Component, public remote_base::RemoteReceiverListener {
 public:
  void dump_config() override;
  bool on_receive(remote_base::RemoteReceiveData data) override;

  void set_dedup_window(uint32_t dedup_window_ms) { this->dedup_window_ms_ = dedup_window_ms; }
  void add_on_frame_callback(std::function<void(SomfyRtsFrame)> &&callback) {
    this->frame_callback_.add(std::move(callback));
  }

 protected:
  CallbackManager<void(SomfyRtsFrame)> frame_callback_{};
  uint32_t dedup_window_ms_{2000};

  bool have_last_{false};
  somfy_rts::Command last_command_{};
  uint16_t last_rolling_code_{0};
  uint32_t last_remote_code_{0};
  uint32_t last_decode_ms_{0};
};

class SomfyRtsFrameTrigger : public Trigger<SomfyRtsFrame> {
 public:
  explicit SomfyRtsFrameTrigger(SomfyRtsReceiver *parent) {
    parent->add_on_frame_callback([this](SomfyRtsFrame frame) { this->trigger(frame); });
  }
};

}  // namespace somfy_rts_receiver
}  // namespace esphome
