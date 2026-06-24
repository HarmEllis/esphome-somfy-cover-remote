// Host round-trip test for the Somfy RTS protocol logic.
//
// Validates that decode_somfy_frame() round-trips build_somfy_timings() across
// all commands and a wide range of rolling/remote codes (it exercises the
// sampled synthetic cases below; it does not claim to cover real-receiver
// behaviour, every timing tolerance, or capture splitting), under three signal
// models:
//   1. raw      - timings exactly as emitted by build_somfy_timings()
//   2. merged   - consecutive same-level pulses collapsed into one, as a real
//                 receiver reports edges (this is where the tricky cases live:
//                 the sync-low merging into bit 0, and a trailing '0' bit's low
//                 merging into the inter-frame gap)
//   3. jittered - merged, then each duration perturbed to emulate RF timing
//                 tolerance
//
// Build & run (no ESPHome toolchain needed):
//   g++ -std=c++17 -O2 -Wall -Wextra test/decode_test.cpp -o /tmp/decode_test
//   /tmp/decode_test

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "../esphome/components/somfy_rts_protocol/somfy_rts_protocol.h"

using namespace esphome::somfy_rts;

static int g_failures = 0;

#define CHECK(cond, ...)                       \
  do {                                         \
    if (!(cond)) {                             \
      g_failures++;                            \
      std::printf("FAIL: " __VA_ARGS__);       \
      std::printf("  (%s:%d)\n", __FILE__, __LINE__); \
    }                                          \
  } while (0)

// Emit one press: a first frame (wake-up + 2 hardware sync) followed by
// `repeats` repeat frames (7 hardware sync), exactly like SomfyRts::send_command_impl_.
static std::vector<int32_t> emit_press(Command cmd, uint16_t rolling, uint32_t remote, int repeats) {
  uint8_t frame[7];
  build_somfy_frame(frame, cmd, rolling, remote);
  std::vector<int32_t> t;
  build_somfy_timings(t, frame, 2);
  for (int i = 0; i < repeats; i++)
    build_somfy_timings(t, frame, 7);
  return t;
}

// Collapse consecutive same-sign pulses, as an edge-based receiver would.
static std::vector<int32_t> merge_edges(const std::vector<int32_t> &t) {
  std::vector<int32_t> out;
  for (int32_t v : t) {
    if (!out.empty() && ((out.back() < 0) == (v < 0)))
      out.back() += v;
    else
      out.push_back(v);
  }
  return out;
}

// Deterministic pseudo-random jitter in [-amp, +amp], applied per pulse while
// preserving sign.
static std::vector<int32_t> add_jitter(const std::vector<int32_t> &t, int amp, uint32_t &seed) {
  std::vector<int32_t> out;
  out.reserve(t.size());
  for (int32_t v : t) {
    seed = seed * 1103515245u + 12345u;
    int delta = static_cast<int>((seed >> 16) % (2 * amp + 1)) - amp;
    int32_t mag = (v < 0 ? -v : v) + delta;
    if (mag < 1)
      mag = 1;
    out.push_back(v < 0 ? -mag : mag);
  }
  return out;
}

static void expect_decode(const std::vector<int32_t> &t, const char *model, Command cmd, uint16_t rolling,
                          uint32_t remote, int repeats) {
  DecodedFrame f;
  bool ok = decode_somfy_frame(t, &f);
  CHECK(ok, "[%s] decode failed for cmd=0x%X rc=0x%04X remote=0x%06X\n", model, (unsigned) cmd, rolling, remote);
  if (!ok)
    return;
  CHECK(f.command == cmd, "[%s] command 0x%X != 0x%X\n", model, (unsigned) f.command, (unsigned) cmd);
  CHECK(f.rolling_code == rolling, "[%s] rolling 0x%04X != 0x%04X\n", model, f.rolling_code, rolling);
  CHECK(f.remote_code == remote, "[%s] remote 0x%06X != 0x%06X\n", model, f.remote_code, remote);
  CHECK(f.repeat_count == repeats, "[%s] repeat_count %u != %d\n", model, (unsigned) f.repeat_count, repeats);
}

int main() {
  const Command commands[] = {Command::My, Command::Up, Command::Down, Command::Prog};
  uint32_t seed = 0xC0FFEE;
  int cases = 0;

  for (Command cmd : commands) {
    // Sweep rolling codes (covers final-bit-0 / final-bit-1 and many merge
    // patterns) and a couple of remote addresses; vary the repeat count.
    for (uint32_t rolling = 0; rolling <= 0xFFFF; rolling += 257) {
      for (uint32_t remote : {0x000000u, 0x6b2a03u, 0xFFFFFFu, 0x123456u}) {
        int repeats = (int) (rolling % 8);  // 0..7 repeat frames
        auto raw = emit_press(cmd, rolling, remote, repeats);
        auto merged = merge_edges(raw);
        auto jittered = add_jitter(merged, 120, seed);

        expect_decode(raw, "raw", cmd, rolling, remote, repeats);
        expect_decode(merged, "merged", cmd, rolling, remote, repeats);
        expect_decode(jittered, "jitter", cmd, rolling, remote, repeats);
        cases += 3;
      }
    }
  }

  // Noise / non-frame input must not falsely decode.
  {
    std::vector<int32_t> noise = {1000, -2000, 600, -600, 5000, -700, 300, -100000};
    DecodedFrame f;
    CHECK(!decode_somfy_frame(noise, &f), "noise unexpectedly decoded\n");
    cases++;
  }

  // A checksum-valid frame carrying an unsupported command nibble must be
  // rejected (the encoder never produces command 0x0).
  {
    uint8_t frame[7];
    build_somfy_frame(frame, static_cast<Command>(0x0), 0x1234, 0xABCDEF);
    std::vector<int32_t> t;
    build_somfy_timings(t, frame, 2);
    DecodedFrame f;
    CHECK(!decode_somfy_frame(t, &f), "frame with unsupported command 0x0 unexpectedly decoded\n");
    cases++;
  }

  std::printf("ran %d cases, %d failures\n", cases, g_failures);
  return g_failures == 0 ? 0 : 1;
}
