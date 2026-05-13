import esphome.config_validation as cv

CONFIG_SCHEMA = cv.invalid(
    "The `cover: - platform: somfy_rts` platform is not supported. "
    "Use `somfy_rts:` as a command-layer component and wire it into "
    "`cover: - platform: time_based` actions. "
    "See MIGRATION.md in this repository."
)
