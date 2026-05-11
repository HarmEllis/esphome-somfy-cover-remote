#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/components/button/button.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"
#include "esphome/components/remote_base/remote_base.h"

// ESPHome moved TimeBasedCover in 2026.x; keep both include paths for compatibility.
#if defined(__has_include)
#if __has_include("esphome/components/time_based/cover/time_based_cover.h")
#include "esphome/components/time_based/cover/time_based_cover.h"
#elif __has_include("esphome/components/time_based/time_based_cover.h")
#include "esphome/components/time_based/time_based_cover.h"
#else
#error "ESPHome TimeBasedCover header not found"
#endif
#else
#include "esphome/components/time_based/time_based_cover.h"
#endif

#include "RollingCodeStorage.h"
#include "NVSRollingCodeStorage.h"

#define COVER_OPEN 1.0f
#define COVER_CLOSED 0.0f

namespace esphome {
namespace somfy_cover {

static const uint16_t SYMBOL = 640;  // microseconds

enum class Command : uint8_t {
  My = 0x1,
  Up = 0x2,
  Down = 0x4,
  Prog = 0x8,
};

// Helper class to attach cover functions to the time based cover triggers
template<typename... Ts> class SomfyCoverAction : public Action<Ts...> {
 public:
  std::function<void(Ts...)> callback;

  explicit SomfyCoverAction(std::function<void(Ts...)> callback) : callback(callback) {}

  void play(Ts... x) override {
    if (callback)
      callback(x...);
  }
};

class SomfyCover : public time_based::TimeBasedCover {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  // Set time based cover values
  void set_open_duration(uint32_t open_duration) { this->open_duration_ = open_duration; }
  void set_close_duration(uint32_t close_duration) { this->close_duration_ = close_duration; }

  // Set remote transmitter
  void set_remote_transmitter(remote_transmitter::RemoteTransmitterComponent *transmitter) {
    this->remote_transmitter_ = transmitter;
  }

  // Set somfy cover button and values
  void set_prog_button(button::Button *cover_prog_button) { this->cover_prog_button_ = cover_prog_button; }
  void set_remote_code(uint32_t remote_code) { this->remote_code_ = remote_code; }
  void set_storage_key(const char *storage_key) { this->storage_key_ = storage_key; }
  void set_storage_namespace(const char *storage_namespace) { this->storage_namespace_ = storage_namespace; }
  void set_repeat_count(int repeat_count) { this->repeat_count_ = repeat_count; }

  cover::CoverTraits get_traits() override;

 protected:
  void control(const cover::CoverCall &call) override;

  // Set via the ESPHome yaml
  remote_transmitter::RemoteTransmitterComponent *remote_transmitter_{nullptr};
  button::Button *cover_prog_button_{nullptr};
  uint32_t remote_code_{0};
  const char *storage_key_{nullptr};
  const char *storage_namespace_{"somfy_cover"};
  int repeat_count_{4};

  // Rolling code storage
  RollingCodeStorage *storage_{nullptr};

  void open();
  void close();
  void stop();
  void program();

  // Somfy RTS protocol
  void send_command(Command command);
  void build_frame(uint8_t *frame, Command command, uint16_t rolling_code);
  void build_timings(remote_base::RawTimings &t, uint8_t *frame, uint8_t sync_count);
  static void send_high(remote_base::RawTimings &t, int32_t duration_usecs);
  static void send_low(remote_base::RawTimings &t, int32_t duration_usecs);

  // Create automations to attach the cover control functions
  Automation<> *automationTriggerUp_{nullptr};
  SomfyCoverAction<> *actionTriggerUp_{nullptr};
  Automation<> *automationTriggerDown_{nullptr};
  SomfyCoverAction<> *actionTriggerDown_{nullptr};
  Automation<> *automationTriggerStop_{nullptr};
  SomfyCoverAction<> *actionTriggerStop_{nullptr};
};

}  // namespace somfy_cover
}  // namespace esphome
