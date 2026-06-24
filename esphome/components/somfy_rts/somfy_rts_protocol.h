#pragma once

// Pure Somfy RTS wire-format logic, shared by the transmit path (SomfyRts) and
// the future receive path (somfy_rts_receiver). This header intentionally has
// no ESPHome dependencies (only <cstdint>/<vector>) so the encode/decode logic
// is a single source of truth and can be unit-tested on the host.
//
// remote_base::RawTimings is std::vector<int32_t> (positive = mark/high,
// negative = space/low, microseconds), so these functions operate on that type
// directly without pulling in remote_base.

#include <cstdint>
#include <vector>

namespace esphome {
namespace somfy_rts {

static const int32_t SYMBOL = 640;  // microseconds (half of one Manchester bit)

enum class Command : uint8_t {
  My = 0x1,
  Up = 0x2,
  Down = 0x4,
  Prog = 0x8,
};

struct DecodedFrame {
  Command command;
  uint16_t rolling_code;
  uint32_t remote_code;
  // Number of repeat frames that followed the first frame within the same
  // capture (i.e. frames sharing this remote_code + rolling_code). This is a
  // best-effort hold-duration hint, not a reliable intent signal: it depends on
  // how long the button was held and on RF reception quality.
  uint8_t repeat_count;
};

// --- Encode ---------------------------------------------------------------

inline void somfy_push_high(std::vector<int32_t> &t, int32_t duration_usecs) { t.push_back(duration_usecs); }
inline void somfy_push_low(std::vector<int32_t> &t, int32_t duration_usecs) { t.push_back(-duration_usecs); }

// Build the 7-byte obfuscated frame for a command + rolling code (inverse of
// the decode step below).
inline void build_somfy_frame(uint8_t *frame, Command command, uint16_t rolling_code, uint32_t remote_code) {
  const uint8_t button = static_cast<uint8_t>(command);

  frame[0] = 0xA7;               // Encryption key
  frame[1] = button << 4;        // Button (high nibble), checksum placeholder (low nibble)
  frame[2] = rolling_code >> 8;  // Rolling code MSB
  frame[3] = rolling_code;       // Rolling code LSB
  frame[4] = remote_code >> 16;  // Remote address byte 0
  frame[5] = remote_code >> 8;   // Remote address byte 1
  frame[6] = remote_code;        // Remote address byte 2

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
}

// Append the raw timings for one frame. sync_count == 2 emits the leading
// wake-up pulse used for the first frame; repeat frames use 7.
inline void build_somfy_timings(std::vector<int32_t> &t, const uint8_t *frame, uint8_t sync_count) {
  // Wake-up pulse (only for first frame, sync_count == 2)
  if (sync_count == 2) {
    somfy_push_high(t, 9415);
    somfy_push_low(t, 9565 + 80000);
  }

  // Hardware sync pulses
  for (uint8_t i = 0; i < sync_count; i++) {
    somfy_push_high(t, 4 * SYMBOL);
    somfy_push_low(t, 4 * SYMBOL);
  }

  // Software sync
  somfy_push_high(t, 4550);
  somfy_push_low(t, SYMBOL);

  // Data: 56 bits (7 bytes), Manchester encoding
  for (uint8_t i = 0; i < 56; i++) {
    if (((frame[i / 8] >> (7 - (i % 8))) & 1) == 1) {
      somfy_push_low(t, SYMBOL);
      somfy_push_high(t, SYMBOL);
    } else {
      somfy_push_high(t, SYMBOL);
      somfy_push_low(t, SYMBOL);
    }
  }

  // Inter-frame gap
  somfy_push_low(t, 415 + 30000);
}

// --- Decode ---------------------------------------------------------------

// Number of SYMBOL-wide half-bits a pulse spans (1 or 2 inside the data; >= 3
// marks a sync pulse or the inter-frame gap, i.e. the end of a data burst).
inline int somfy_half_symbols(int32_t duration_usecs) {
  int32_t d = duration_usecs < 0 ? -duration_usecs : duration_usecs;
  return (d + SYMBOL / 2) / SYMBOL;
}

// Decode a single frame whose software sync (~4550 us high) is at timings[sync].
// Returns true and fills *out (command/rolling_code/remote_code; repeat_count
// untouched) when the nibble checksum validates.
inline bool decode_somfy_frame_at(const std::vector<int32_t> &timings, size_t sync, DecodedFrame *out) {
  const size_t n = timings.size();

  // Expand the pulses after the software sync into half-bits, stopping at the
  // inter-frame gap or the next frame's hardware sync (>= 3 half-symbols).
  std::vector<bool> hb;  // true = HIGH half-bit
  for (size_t j = sync + 1; j < n; j++) {
    int hs = somfy_half_symbols(timings[j]);
    if (hs >= 3 || hs < 1) {
      // A long LOW terminator (the inter-frame gap) can absorb the trailing
      // LOW half-bit of a final '0' bit; restore it so that bit can decode.
      if (timings[j] < 0)
        hb.push_back(false);
      break;
    }
    for (int k = 0; k < hs; k++)
      hb.push_back(timings[j] > 0);
  }

  // 56 bits = 112 half-bits. The leading sync-low shifts the alignment by one,
  // so phase 1 is the expected alignment; phase 0 is a fallback. The checksum
  // guards against a false lock.
  for (int phase : {1, 0}) {
    if (hb.size() < static_cast<size_t>(phase) + 112)
      continue;

    uint8_t frame[7] = {0};
    bool ok = true;
    for (int bit = 0; bit < 56; bit++) {
      bool first = hb[phase + 2 * bit];
      bool second = hb[phase + 2 * bit + 1];
      if (first == second) {  // not a valid Manchester bit at this phase
        ok = false;
        break;
      }
      // build_somfy_timings encodes a 1 as low-then-high, a 0 as high-then-low.
      if (!first && second)
        frame[bit / 8] |= (1 << (7 - (bit % 8)));
    }
    if (!ok)
      continue;

    // Undo the rolling-XOR obfuscation (reverse of build_somfy_frame).
    for (int b = 6; b >= 1; b--)
      frame[b] ^= frame[b - 1];

    // Verify the nibble checksum.
    uint8_t checksum = frame[1] & 0x0F;
    frame[1] &= 0xF0;
    uint8_t calc = 0;
    for (int b = 0; b < 7; b++)
      calc ^= frame[b] ^ (frame[b] >> 4);
    calc &= 0x0F;
    if (calc != checksum)
      continue;

    out->command = static_cast<Command>(frame[1] >> 4);
    out->rolling_code = (frame[2] << 8) | frame[3];
    out->remote_code = (static_cast<uint32_t>(frame[4]) << 16) | (frame[5] << 8) | frame[6];
    return true;
  }
  return false;
}

// Decode the first checksum-valid frame in a capture and report how many repeat
// frames followed it (frames sharing the same remote_code + rolling_code +
// command). Returns false when no valid frame is present.
inline bool decode_somfy_frame(const std::vector<int32_t> &timings, DecodedFrame *out) {
  bool found = false;
  uint8_t matches = 0;
  for (size_t i = 0; i + 1 < timings.size(); i++) {
    // Lock onto the software sync: a ~4550 us HIGH pulse.
    if (timings[i] < 4000 || timings[i] > 5200)
      continue;

    DecodedFrame f;
    if (!decode_somfy_frame_at(timings, i, &f))
      continue;

    if (!found) {
      *out = f;
      found = true;
      matches = 1;
    } else if (f.command == out->command && f.rolling_code == out->rolling_code &&
               f.remote_code == out->remote_code) {
      if (matches < 255)
        matches++;
    }
  }
  if (found)
    out->repeat_count = matches - 1;
  return found;
}

}  // namespace somfy_rts
}  // namespace esphome
