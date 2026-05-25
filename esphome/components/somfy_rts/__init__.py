import esphome.codegen as cg
from esphome import automation
from esphome.components import button, remote_transmitter
from esphome.automation import maybe_simple_id
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@HarmEllis"]
DEPENDENCIES = ["remote_transmitter"]
MULTI_CONF = True

somfy_rts_ns = cg.esphome_ns.namespace("somfy_rts")
SomfyRts = somfy_rts_ns.class_("SomfyRts", cg.Component)

Command = somfy_rts_ns.enum("Command", is_class=True)
COMMAND_MAP = {
    "UP": Command.Up,
    "DOWN": Command.Down,
    "MY": Command.My,
    "PROG": Command.Prog,
}

OpenAction = somfy_rts_ns.class_("OpenAction", automation.Action)
CloseAction = somfy_rts_ns.class_("CloseAction", automation.Action)
StopAction = somfy_rts_ns.class_("StopAction", automation.Action)
ProgramAction = somfy_rts_ns.class_("ProgramAction", automation.Action)
OpenTiltAction = somfy_rts_ns.class_("OpenTiltAction", automation.Action)
CloseTiltAction = somfy_rts_ns.class_("CloseTiltAction", automation.Action)
SendAction = somfy_rts_ns.class_("SendAction", automation.Action)

CONF_REMOTE_TRANSMITTER = "remote_transmitter"
CONF_PROG_BUTTON = "prog_button"
CONF_REMOTE_CODE = "remote_code"
CONF_SOMFY_STORAGE_KEY = "storage_key"
CONF_SOMFY_STORAGE_NAMESPACE = "storage_namespace"
CONF_REPEAT_COMMAND_COUNT = "repeat_command_count"
CONF_TILT_REPEAT_COUNT = "tilt_repeat_count"
CONF_COMMAND = "command"
CONF_REPEAT_COUNT = "repeat_count"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(SomfyRts),
        cv.Required(CONF_REMOTE_TRANSMITTER): cv.use_id(
            remote_transmitter.RemoteTransmitterComponent
        ),
        cv.Required(CONF_REMOTE_CODE): cv.uint32_t,
        cv.Required(CONF_SOMFY_STORAGE_KEY): cv.All(cv.string, cv.Length(max=15)),
        cv.Optional(CONF_SOMFY_STORAGE_NAMESPACE, default="somfy_rts"): cv.All(
            cv.string, cv.Length(max=15)
        ),
        cv.Optional(CONF_REPEAT_COMMAND_COUNT, default=4): cv.int_range(min=1, max=100),
        cv.Optional(CONF_TILT_REPEAT_COUNT, default=3): cv.int_range(min=1, max=100),
        cv.Optional(CONF_PROG_BUTTON): cv.use_id(button.Button),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    transmitter = await cg.get_variable(config[CONF_REMOTE_TRANSMITTER])
    cg.add(var.set_remote_transmitter(transmitter))

    if prog_button := config.get(CONF_PROG_BUTTON):
        btn = await cg.get_variable(prog_button)
        cg.add(var.set_prog_button(btn))

    cg.add(var.set_remote_code(config[CONF_REMOTE_CODE]))
    cg.add(var.set_storage_key(config[CONF_SOMFY_STORAGE_KEY]))
    cg.add(var.set_storage_namespace(config[CONF_SOMFY_STORAGE_NAMESPACE]))
    cg.add(var.set_repeat_count(config[CONF_REPEAT_COMMAND_COUNT]))
    cg.add(var.set_tilt_repeat_count(config[CONF_TILT_REPEAT_COUNT]))


SOMFY_ACTION_SCHEMA = cv.Schema(
    maybe_simple_id({cv.GenerateID(CONF_ID): cv.use_id(SomfyRts)})
)


@automation.register_action(
    "somfy_rts.open", OpenAction, SOMFY_ACTION_SCHEMA, synchronous=True
)
@automation.register_action(
    "somfy_rts.close", CloseAction, SOMFY_ACTION_SCHEMA, synchronous=True
)
@automation.register_action(
    "somfy_rts.stop", StopAction, SOMFY_ACTION_SCHEMA, synchronous=True
)
@automation.register_action(
    "somfy_rts.program", ProgramAction, SOMFY_ACTION_SCHEMA, synchronous=True
)
@automation.register_action(
    "somfy_rts.open_tilt", OpenTiltAction, SOMFY_ACTION_SCHEMA, synchronous=True
)
@automation.register_action(
    "somfy_rts.close_tilt", CloseTiltAction, SOMFY_ACTION_SCHEMA, synchronous=True
)
async def somfy_action_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


SOMFY_SEND_ACTION_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.use_id(SomfyRts),
        cv.Required(CONF_COMMAND): cv.enum(COMMAND_MAP, upper=True),
        cv.Optional(CONF_REPEAT_COUNT): cv.templatable(cv.int_range(min=1, max=100)),
    }
)


@automation.register_action(
    "somfy_rts.send", SendAction, SOMFY_SEND_ACTION_SCHEMA, synchronous=True
)
async def somfy_send_action_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    cg.add(var.set_command(config[CONF_COMMAND]))
    if CONF_REPEAT_COUNT in config:
        templ = await cg.templatable(config[CONF_REPEAT_COUNT], args, cg.int_)
        cg.add(var.set_repeat_count(templ))
    return var
