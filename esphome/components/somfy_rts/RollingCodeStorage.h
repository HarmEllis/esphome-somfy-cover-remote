#pragma once

#include <cstdint>

namespace esphome {
namespace somfy_rts {

class RollingCodeStorage {
 public:
  virtual ~RollingCodeStorage() = default;
  virtual uint16_t next_code() = 0;
};

}  // namespace somfy_rts
}  // namespace esphome
