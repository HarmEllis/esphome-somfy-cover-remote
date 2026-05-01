#include "NVSRollingCodeStorage.h"
#include "esphome/core/log.h"
#include <nvs_flash.h>
#include <nvs.h>

namespace esphome {
namespace somfy_cover {

static const char *TAG = "somfy_cover.nvs";

uint16_t NVSRollingCodeStorage::next_code() {
  uint16_t code = 1000; // see also nvs_get_u16
  nvs_handle_t handle;

  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_LOGW(TAG, "NVS partition truncated, erasing...");
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to init NVS: %s", esp_err_to_name(err));
    return code;
  }

  err = nvs_open(this->ns_, NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open NVS namespace '%s': %s", this->ns_, esp_err_to_name(err));
    return code;
  }

  err = nvs_get_u16(handle, this->key_, &code);
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    code = 1000;
  } else if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read rolling code: %s", esp_err_to_name(err));
    nvs_close(handle);
    return code;
  }

  err = nvs_set_u16(handle, this->key_, code + 1);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to write rolling code: %s", esp_err_to_name(err));
  }

  nvs_commit(handle);
  nvs_close(handle);

  ESP_LOGD(TAG, "Rolling code for '%s': %u", this->key_, code);
  return code;
}

}  // namespace somfy_cover
}  // namespace esphome
