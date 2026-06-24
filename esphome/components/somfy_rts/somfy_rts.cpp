#include "esphome/core/log.h"
#include "somfy_rts.h"

namespace esphome {
namespace somfy_rts {

static const char *TAG = "somfy_rts.component";

static void log_component_action(const char *action, uint32_t remote_code) {
  ESP_LOGI(TAG, "%s remote=0x%06X", action, remote_code);
}

void SomfyRts::setup() {
  this->storage_ = new NVSRollingCodeStorage(this->storage_namespace_, this->storage_key_);

  if (this->cover_prog_button_ != nullptr) {
    this->cover_prog_button_->add_on_press_callback([this] { this->program(); });
  }
}

void SomfyRts::dump_config() {
  ESP_LOGCONFIG(TAG, "Somfy RTS command layer:");
  ESP_LOGCONFIG(TAG, "  Remote code: 0x%06X", this->remote_code_);
  ESP_LOGCONFIG(TAG, "  Storage namespace: %s", this->storage_namespace_);
  ESP_LOGCONFIG(TAG, "  Storage key: %s", this->storage_key_);
  ESP_LOGCONFIG(TAG, "  Repeat count: %d", this->repeat_count_);
  ESP_LOGCONFIG(TAG, "  Tilt repeat count: %d", this->tilt_repeat_count_);
  ESP_LOGCONFIG(TAG, "  Prog button attached: %s", YESNO(this->cover_prog_button_ != nullptr));
}

void SomfyRts::open() {
  log_component_action("OPEN", this->remote_code_);
  this->send_command(Command::Up);
}

void SomfyRts::close() {
  log_component_action("CLOSE", this->remote_code_);
  this->send_command(Command::Down);
}

void SomfyRts::stop() {
  log_component_action("STOP", this->remote_code_);
  this->send_command(Command::My);
}

void SomfyRts::program() {
  log_component_action("PROG", this->remote_code_);
  this->send_command(Command::Prog);
}

void SomfyRts::open_tilt() {
  log_component_action("OPEN_TILT", this->remote_code_);
  this->send_command(Command::Up, this->tilt_repeat_count_);
}

void SomfyRts::close_tilt() {
  log_component_action("CLOSE_TILT", this->remote_code_);
  this->send_command(Command::Down, this->tilt_repeat_count_);
}

void SomfyRts::send_command(Command command) {
  this->send_command_impl_(command, this->repeat_count_);
}

void SomfyRts::send_command(Command command, int repeat_count) {
  if (repeat_count < 0) {
    ESP_LOGW(TAG, "repeat_count %d below 0, clamping to 0", repeat_count);
    repeat_count = 0;
  } else if (repeat_count > 100) {
    ESP_LOGW(TAG, "repeat_count %d above 100, clamping to 100", repeat_count);
    repeat_count = 100;
  }
  this->send_command_impl_(command, repeat_count);
}

void SomfyRts::send_command_impl_(Command command, int repeat_count) {
  if (this->remote_transmitter_ == nullptr) {
    ESP_LOGE(TAG, "No remote_transmitter configured");
    return;
  }

  if (this->storage_ == nullptr) {
    ESP_LOGE(TAG, "Rolling code storage is not initialized");
    return;
  }

  ESP_LOGD(TAG, "Repeat count: %d", repeat_count);

  const uint16_t rolling_code = this->storage_->next_code();

  uint8_t frame[7];
  build_somfy_frame(frame, command, rolling_code, this->remote_code_);

  ESP_LOGD(TAG, "Frame: %02X %02X %02X %02X %02X %02X %02X", frame[0], frame[1], frame[2], frame[3], frame[4],
           frame[5], frame[6]);

  remote_base::RawTimings timings;

  // First frame with wake-up and 2 hardware sync pulses
  build_somfy_timings(timings, frame, 2);

  // Repeat frames with 7 hardware sync pulses
  for (int i = 0; i < repeat_count; i++) {
    build_somfy_timings(timings, frame, 7);
  }

  auto call = this->remote_transmitter_->transmit();
  call.get_data()->set_data(timings);
  call.perform();
}

}  // namespace somfy_rts
}  // namespace esphome
