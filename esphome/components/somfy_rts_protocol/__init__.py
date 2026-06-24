import esphome.codegen as cg  # noqa: F401

# Header-only component: the pure Somfy RTS wire-format logic lives in
# somfy_rts_protocol.h with no ESPHome dependencies, so it can be shared as a
# single source of truth by both the transmit (somfy_rts) and receive
# (somfy_rts_receiver) components and unit-tested on the host. There is nothing
# to configure or generate here; it is pulled in via AUTO_LOAD.
CODEOWNERS = ["@HarmEllis"]
