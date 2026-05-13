#pragma once

#include "RollingCodeStorage.h"

namespace esphome {
namespace somfy_rts {

class NVSRollingCodeStorage : public RollingCodeStorage {
 public:
  NVSRollingCodeStorage(const char *ns, const char *key) : ns_(ns), key_(key) {}
  uint16_t next_code() override;

 private:
  const char *ns_;
  const char *key_;
};

}  // namespace somfy_rts
}  // namespace esphome
