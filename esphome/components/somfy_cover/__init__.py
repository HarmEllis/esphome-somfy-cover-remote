import esphome.config_validation as cv

CONFIG_SCHEMA = cv.invalid(
    "The `somfy_cover:` component has been renamed to `somfy_rts:`. "
    "Use `somfy_rts:` as the command layer and `cover: - platform: time_based` "
    "for cover behavior. See MIGRATION.md in this repository."
)
