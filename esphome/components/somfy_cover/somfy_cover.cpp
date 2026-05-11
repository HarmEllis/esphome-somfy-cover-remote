#include <array>
#include "esphome/core/log.h"
#include "somfy_cover.h"

namespace esphome {
namespace somfy_cover {

static const char *TAG = "somfy_cover.cover";

static void log_cover_action(const char *action, EntityBase *entity) {
  std::array<char, OBJECT_ID_MAX_LEN> object_id_buf{};
  StringRef object_id = entity->get_object_id_to(object_id_buf);
  ESP_LOGI(TAG, "%s %s", action, object_id.c_str());
}

void SomfyCover::setup() {
  // Setup cover rolling code storage
  this->storage_ = new NVSRollingCodeStorage(this->storage_namespace_, this->storage_key_);

  // Attach to timebased cover controls
  automationTriggerUp_ = new Automation<>(this->get_open_trigger());
  actionTriggerUp_ = new SomfyCoverAction<>([this] { this->open(); });
  automationTriggerUp_->add_action(actionTriggerUp_);

  automationTriggerDown_ = new Automation<>(this->get_close_trigger());
  actionTriggerDown_ = new SomfyCoverAction<>([this] { this->close(); });
  automationTriggerDown_->add_action(actionTriggerDown_);

  automationTriggerStop_ = new Automation<>(this->get_stop_trigger());
  actionTriggerStop_ = new SomfyCoverAction<>([this] { this->stop(); });
  automationTriggerStop_->add_action(actionTriggerStop_);

  // Attach the prog button
  this->cover_prog_button_->add_on_press_callback([this] { this->program(); });

  // Set extra settings
  this->has_built_in_endstop_ = true;
  this->assumed_state_ = true;

  TimeBasedCover::setup();
}

void SomfyCover::loop() { TimeBasedCover::loop(); }

void SomfyCover::dump_config() {
  ESP_LOGCONFIG(TAG, "Somfy cover:");
  ESP_LOGCONFIG(TAG, "  Remote code: 0x%06X", this->remote_code_);
  ESP_LOGCONFIG(TAG, "  Storage namespace: %s", this->storage_namespace_);
  ESP_LOGCONFIG(TAG, "  Storage key: %s", this->storage_key_);
  ESP_LOGCONFIG(TAG, "  Repeat count: %d", this->repeat_count_);
}

cover::CoverTraits SomfyCover::get_traits() {
  auto traits = TimeBasedCover::get_traits();
  traits.set_supports_tilt(false);
  return traits;
}

void SomfyCover::control(const cover::CoverCall &call) { TimeBasedCover::control(call); }

void SomfyCover::open() {
  log_cover_action("OPEN", this);
  this->send_command(Command::Up);
}

void SomfyCover::close() {
  log_cover_action("CLOSE", this);
  this->send_command(Command::Down);
}

void SomfyCover::stop() {
  log_cover_action("STOP", this);
  this->send_command(Command::My);
}

void SomfyCover::program() {
  log_cover_action("PROG", this);
  this->send_command(Command::Prog);
}

void SomfyCover::send_command(Command command) {
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

void SomfyCover::build_frame(uint8_t *frame, Command command, uint16_t rolling_code) {
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

  ESP_LOGD(TAG, "Frame: %02X %02X %02X %02X %02X %02X %02X",
           frame[0], frame[1], frame[2], frame[3], frame[4], frame[5], frame[6]);
}

void SomfyCover::build_timings(remote_base::RawTimings &t, uint8_t *frame, uint8_t sync_count) {
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

void SomfyCover::send_high(remote_base::RawTimings &t, int32_t duration_usecs) {
  t.push_back(duration_usecs);
}

void SomfyCover::send_low(remote_base::RawTimings &t, int32_t duration_usecs) {
  t.push_back(-duration_usecs);
}

}  // namespace somfy_cover
}  // namespace esphome
