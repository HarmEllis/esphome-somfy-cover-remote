#include <array>

#include "esphome/core/log.h"
#include "somfy_rts.h"

namespace esphome {
namespace somfy_rts {

static const char *TAG = "somfy_rts.component";

static void log_component_action(const char *action, EntityBase *entity) {
  std::array<char, OBJECT_ID_MAX_LEN> object_id_buf{};
  StringRef object_id = entity->get_object_id_to(object_id_buf);
  ESP_LOGI(TAG, "%s %s", action, object_id.c_str());
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
  ESP_LOGCONFIG(TAG, "  Prog button attached: %s", YESNO(this->cover_prog_button_ != nullptr));
}

void SomfyRts::open() {
  log_component_action("OPEN", this);
  this->send_command(Command::Up);
}

void SomfyRts::close() {
  log_component_action("CLOSE", this);
  this->send_command(Command::Down);
}

void SomfyRts::stop() {
  log_component_action("STOP", this);
  this->send_command(Command::My);
}

void SomfyRts::program() {
  log_component_action("PROG", this);
  this->send_command(Command::Prog);
}

void SomfyRts::send_command(Command command) {
  if (this->remote_transmitter_ == nullptr) {
    ESP_LOGE(TAG, "No remote_transmitter configured");
    return;
  }

  if (this->storage_ == nullptr) {
    ESP_LOGE(TAG, "Rolling code storage is not initialized");
    return;
  }

  const uint16_t rolling_code = this->storage_->next_code();

  uint8_t frame[7];
  this->build_frame(frame, command, rolling_code);

  remote_base::RawTimings timings;

  // First frame with wake-up and 2 hardware sync pulses
  this->build_timings(timings, frame, 2);

  // Repeat frames with 7 hardware sync pulses
  for (int i = 0; i < this->repeat_count_; i++) {
    this->build_timings(timings, frame, 7);
  }

  auto call = this->remote_transmitter_->transmit();
  call.get_data()->set_data(timings);
  call.perform();
}

void SomfyRts::build_frame(uint8_t *frame, Command command, uint16_t rolling_code) {
  const uint8_t button = static_cast<uint8_t>(command);

  frame[0] = 0xA7;              // Encryption key
  frame[1] = button << 4;       // Button (high nibble), checksum placeholder (low nibble)
  frame[2] = rolling_code >> 8; // Rolling code MSB
  frame[3] = rolling_code;      // Rolling code LSB
  frame[4] = this->remote_code_ >> 16;  // Remote address byte 0
  frame[5] = this->remote_code_ >> 8;   // Remote address byte 1
  frame[6] = this->remote_code_;        // Remote address byte 2

  // Checksum: XOR of all nibbles
  uint8_t checksum = 0;
  for (uint8_t i = 0; i < 7; i++) {
    checksum = checksum ^ frame[i] ^ (frame[i] >> 4);
  }
  checksum &= 0x0F;
  frame[1] |= checksum;

  // Obfuscation: rolling XOR
  for (uint8_t i = 1; i < 7; i++) {
    frame[i] ^= frame[i - 1];
  }

  ESP_LOGD(TAG, "Frame: %02X %02X %02X %02X %02X %02X %02X", frame[0], frame[1], frame[2], frame[3], frame[4],
           frame[5], frame[6]);
}

void SomfyRts::build_timings(remote_base::RawTimings &t, uint8_t *frame, uint8_t sync_count) {
  // Wake-up pulse (only for first frame, sync_count == 2)
  if (sync_count == 2) {
    send_high(t, 9415);
    send_low(t, 9565 + 80000);
  }

  // Hardware sync pulses
  for (uint8_t i = 0; i < sync_count; i++) {
    send_high(t, 4 * SYMBOL);
    send_low(t, 4 * SYMBOL);
  }

  // Software sync
  send_high(t, 4550);
  send_low(t, SYMBOL);

  // Data: 56 bits (7 bytes), Manchester encoding
  for (uint8_t i = 0; i < 56; i++) {
    if (((frame[i / 8] >> (7 - (i % 8))) & 1) == 1) {
      send_low(t, SYMBOL);
      send_high(t, SYMBOL);
    } else {
      send_high(t, SYMBOL);
      send_low(t, SYMBOL);
    }
  }

  // Inter-frame gap
  send_low(t, 415 + 30000);
}

void SomfyRts::send_high(remote_base::RawTimings &t, int32_t duration_usecs) { t.push_back(duration_usecs); }

void SomfyRts::send_low(remote_base::RawTimings &t, int32_t duration_usecs) { t.push_back(-duration_usecs); }

}  // namespace somfy_rts
}  // namespace esphome
