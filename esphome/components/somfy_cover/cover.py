import esphome.codegen as cg
from esphome.components import button, cover, remote_transmitter
import esphome.config_validation as cv
from esphome.const import (
    CONF_CLOSE_DURATION,
    CONF_ID,
    CONF_OPEN_DURATION,
)

CODEOWNERS = ["@HarmEllis"]

AUTO_LOAD = ["button", "time_based"]

DEPENDENCIES = ["esp32"]

somfy_cover_ns = cg.esphome_ns.namespace("somfy_cover")
SomfyCover = somfy_cover_ns.class_("SomfyCover", cover.Cover, cg.Component)

CONF_REMOTE_TRANSMITTER = "remote_transmitter"
CONF_PROG_BUTTON = "prog_button"
CONF_REMOTE_CODE = "remote_code"
CONF_SOMFY_STORAGE_KEY = "storage_key"
CONF_SOMFY_STORAGE_NAMESPACE = "storage_namespace"
CONF_REPEAT_COMMAND_COUNT = "repeat_command_count"

CONFIG_SCHEMA = cv.All(
    cover.cover_schema(SomfyCover)
    .extend(
        {
            cv.GenerateID(): cv.declare_id(SomfyCover),
            cv.Required(CONF_PROG_BUTTON): cv.use_id(button.Button),
            cv.Required(CONF_REMOTE_TRANSMITTER): cv.use_id(
                remote_transmitter.RemoteTransmitterComponent
            ),
            cv.Required(CONF_OPEN_DURATION): cv.positive_time_period_milliseconds,
            cv.Required(CONF_CLOSE_DURATION): cv.positive_time_period_milliseconds,
            cv.Required(CONF_REMOTE_CODE): cv.uint32_t,
            cv.Required(CONF_SOMFY_STORAGE_KEY): cv.All(
                cv.string, cv.Length(max=15)
            ),
            cv.Optional(
                CONF_SOMFY_STORAGE_NAMESPACE, default="somfy_cover"
            ): cv.All(cv.string, cv.Length(max=15)),
            cv.Optional(CONF_REPEAT_COMMAND_COUNT, default=4): cv.int_range(
                min=1, max=100
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cover.register_cover(var, config)

    transmitter = await cg.get_variable(config[CONF_REMOTE_TRANSMITTER])
    cg.add(var.set_remote_transmitter(transmitter))

    btn = await cg.get_variable(config[CONF_PROG_BUTTON])
    cg.add(var.set_prog_button(btn))

    cg.add(var.set_open_duration(config[CONF_OPEN_DURATION]))
    cg.add(var.set_close_duration(config[CONF_CLOSE_DURATION]))
    cg.add(var.set_remote_code(config[CONF_REMOTE_CODE]))
    cg.add(var.set_storage_key(config[CONF_SOMFY_STORAGE_KEY]))
    cg.add(var.set_storage_namespace(config[CONF_SOMFY_STORAGE_NAMESPACE]))
    cg.add(var.set_repeat_count(config[CONF_REPEAT_COMMAND_COUNT]))
