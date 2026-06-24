#include "somfy_rts_receiver.h"
#include "esphome/core/log.h"

namespace esphome {
namespace somfy_rts_receiver {

static const char *const TAG = "somfy_rts_receiver";

void SomfyRtsReceiver::dump_config() {
  ESP_LOGCONFIG(TAG, "Somfy RTS receiver:");
  ESP_LOGCONFIG(TAG, "  Dedup window: %u ms", this->dedup_window_ms_);
}

bool SomfyRtsReceiver::on_receive(remote_base::RemoteReceiveData data) {
  somfy_rts::DecodedFrame frame;
  if (!somfy_rts::decode_somfy_frame(data.get_raw_data(), &frame))
    return false;  // not a Somfy frame (or only a sync burst); let other handlers run

  const uint32_t now = millis();
  const bool repeat = this->have_last_ && frame.command == this->last_command_ &&
                      frame.rolling_code == this->last_rolling_code_ &&
                      frame.remote_code == this->last_remote_code_ &&
                      (now - this->last_decode_ms_) < this->dedup_window_ms_;

  this->have_last_ = true;
  this->last_command_ = frame.command;
  this->last_rolling_code_ = frame.rolling_code;
  this->last_remote_code_ = frame.remote_code;
  this->last_decode_ms_ = now;

  const char *command = somfy_rts::command_to_string(frame.command);
  if (repeat) {
    ESP_LOGV(TAG, "Repeat %s remote=0x%06X rolling=%u", command, frame.remote_code, frame.rolling_code);
    return true;
  }

  ESP_LOGI(TAG, "%s remote=0x%06X rolling=%u", command, frame.remote_code, frame.rolling_code);
  this->frame_callback_.call(SomfyRtsFrame{frame.command, frame.rolling_code, frame.remote_code});
  return true;
}

}  // namespace somfy_rts_receiver
}  // namespace esphome
