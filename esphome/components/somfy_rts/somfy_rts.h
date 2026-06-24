#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/components/button/button.h"
#include "esphome/components/remote_base/remote_base.h"

#include "RollingCodeStorage.h"
#include "NVSRollingCodeStorage.h"

namespace esphome {
namespace somfy_rts {

static const uint16_t SYMBOL = 640;  // microseconds

enum class Command : uint8_t {
  My = 0x1,
  Up = 0x2,
  Down = 0x4,
  Prog = 0x8,
};

class SomfyRts : public Component {
 public:
  void setup() override;
  void dump_config() override;

  // Set via YAML
  void set_remote_transmitter(remote_base::RemoteTransmitterBase *transmitter) { this->remote_transmitter_ = transmitter; }
  void set_prog_button(button::Button *cover_prog_button) { this->cover_prog_button_ = cover_prog_button; }
  void set_remote_code(uint32_t remote_code) { this->remote_code_ = remote_code; }
  void set_storage_key(const char *storage_key) { this->storage_key_ = storage_key; }
  void set_storage_namespace(const char *storage_namespace) { this->storage_namespace_ = storage_namespace; }
  void set_repeat_count(int repeat_count) { this->repeat_count_ = repeat_count; }
  void set_tilt_repeat_count(int tilt_repeat_count) { this->tilt_repeat_count_ = tilt_repeat_count; }

  void open();
  void close();
  void stop();
  void program();
  void open_tilt();
  void close_tilt();
  void send_command(Command command);
  void send_command(Command command, int repeat_count);

 protected:
  remote_base::RemoteTransmitterBase *remote_transmitter_{nullptr};
  button::Button *cover_prog_button_{nullptr};
  uint32_t remote_code_{0};
  const char *storage_key_{nullptr};
  const char *storage_namespace_{"somfy_rts"};
  int repeat_count_{4};
  int tilt_repeat_count_{3};

  RollingCodeStorage *storage_{nullptr};

  void send_command_impl_(Command command, int repeat_count);
  void build_frame(uint8_t *frame, Command command, uint16_t rolling_code);
  void build_timings(remote_base::RawTimings &t, uint8_t *frame, uint8_t sync_count);
  static void send_high(remote_base::RawTimings &t, int32_t duration_usecs);
  static void send_low(remote_base::RawTimings &t, int32_t duration_usecs);
};

template<typename... Ts> class OpenAction : public Action<Ts...> {
 public:
  explicit OpenAction(SomfyRts *parent) : parent_(parent) {}

  void play(const Ts &...) override {
    this->parent_->open();
  }

 protected:
  SomfyRts *parent_;
};

template<typename... Ts> class CloseAction : public Action<Ts...> {
 public:
  explicit CloseAction(SomfyRts *parent) : parent_(parent) {}

  void play(const Ts &...) override {
    this->parent_->close();
  }

 protected:
  SomfyRts *parent_;
};

template<typename... Ts> class StopAction : public Action<Ts...> {
 public:
  explicit StopAction(SomfyRts *parent) : parent_(parent) {}

  void play(const Ts &...) override {
    this->parent_->stop();
  }

 protected:
  SomfyRts *parent_;
};

template<typename... Ts> class ProgramAction : public Action<Ts...> {
 public:
  explicit ProgramAction(SomfyRts *parent) : parent_(parent) {}

  void play(const Ts &...) override {
    this->parent_->program();
  }

 protected:
  SomfyRts *parent_;
};

template<typename... Ts> class OpenTiltAction : public Action<Ts...> {
 public:
  explicit OpenTiltAction(SomfyRts *parent) : parent_(parent) {}

  void play(const Ts &...) override {
    this->parent_->open_tilt();
  }

 protected:
  SomfyRts *parent_;
};

template<typename... Ts> class CloseTiltAction : public Action<Ts...> {
 public:
  explicit CloseTiltAction(SomfyRts *parent) : parent_(parent) {}

  void play(const Ts &...) override {
    this->parent_->close_tilt();
  }

 protected:
  SomfyRts *parent_;
};

template<typename... Ts> class SendAction : public Action<Ts...> {
 public:
  explicit SendAction(SomfyRts *parent) : parent_(parent) {}

  void set_command(Command command) { this->command_ = command; }
  TEMPLATABLE_VALUE(int, repeat_count)

  void play(const Ts &... x) override {
    if (this->repeat_count_.has_value()) {
      this->parent_->send_command(this->command_, this->repeat_count_.value(x...));
    } else {
      this->parent_->send_command(this->command_);
    }
  }

 protected:
  SomfyRts *parent_;
  Command command_{Command::My};
};

}  // namespace somfy_rts
}  // namespace esphome
