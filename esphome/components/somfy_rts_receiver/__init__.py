import esphome.codegen as cg
from esphome import automation
import esphome.config_validation as cv
from esphome.components import remote_base, remote_receiver
from esphome.const import CONF_ID, CONF_TRIGGER_ID

CODEOWNERS = ["@HarmEllis"]
DEPENDENCIES = ["remote_receiver"]
AUTO_LOAD = ["somfy_rts_protocol"]

somfy_rts_receiver_ns = cg.esphome_ns.namespace("somfy_rts_receiver")
SomfyRtsReceiver = somfy_rts_receiver_ns.class_(
    "SomfyRtsReceiver", cg.Component, remote_base.RemoteReceiverListener
)
SomfyRtsFrame = somfy_rts_receiver_ns.struct("SomfyRtsFrame")
SomfyRtsFrameTrigger = somfy_rts_receiver_ns.class_(
    "SomfyRtsFrameTrigger", automation.Trigger.template(SomfyRtsFrame)
)

CONF_REMOTE_RECEIVER = "remote_receiver"
CONF_DEDUP_WINDOW = "dedup_window"
CONF_ON_FRAME = "on_frame"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(SomfyRtsReceiver),
        cv.Required(CONF_REMOTE_RECEIVER): cv.use_id(
            remote_receiver.RemoteReceiverComponent
        ),
        cv.Optional(
            CONF_DEDUP_WINDOW, default="2000ms"
        ): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_ON_FRAME): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(SomfyRtsFrameTrigger),
            }
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    receiver = await cg.get_variable(config[CONF_REMOTE_RECEIVER])
    cg.add(receiver.register_listener(var))
    cg.add(var.set_dedup_window(config[CONF_DEDUP_WINDOW]))

    for conf in config.get(CONF_ON_FRAME, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(SomfyRtsFrame, "x")], conf)
