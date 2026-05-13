# ESPHome Somfy cover remote

This is an external component for [ESPHome](https://esphome.io/) to control Somfy RTS covers from, for example, [Home Assistant](https://www.home-assistant.io/).

It now provides a **Somfy RTS command layer** (`somfy_rts:`) that you wire into the official ESPHome `time_based` cover platform for position control.

## Migration

If you used the old `cover: - platform: somfy_cover` setup, read:

- [MIGRATION.md](./MIGRATION.md)

## Required hardware

- ESP32
- CC1101 RF module

## Setup

Use this as a base and duplicate the command/cover blocks per shutter.

```yaml
esphome:
  name: somfycontroller

external_components:
  - source: github://HarmEllis/esphome-somfy-cover-remote@main
    components: [ somfy_rts ]

esp32:
  board: esp32dev
  framework:
    type: esp-idf

logger:
api:
ota:
  platform: esphome
  password: !secret ota_password

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  ap:
    ssid: "Somfycontroller Fallback Hotspot"
    password: !secret fallback_hotspot

# Optional: template button for pairing
button:
  - platform: template
    id: program_livingroom
    name: "Program livingroom"

# SPI bus for CC1101 (adjust pins for your board)
spi:
  clk_pin: GPIO14
  mosi_pin: GPIO12
  miso_pin: GPIO39

cc1101:
  id: cc1101_radio
  cs_pin: GPIO15
  frequency: 433.42MHz
  modulation_type: ASK/OOK

remote_transmitter:
  id: transmitter
  pin: GPIO2
  carrier_duty_percent: 100%
  on_transmit:
    then:
      - cc1101.begin_tx: cc1101_radio
  on_complete:
    then:
      - cc1101.begin_rx: cc1101_radio

somfy_rts:
  - id: livingroom_cmd
    remote_transmitter: transmitter
    remote_code: 0x6b2a03
    storage_key: "livingroom"
    storage_namespace: "somfy_rts"
    repeat_command_count: 4
    prog_button: program_livingroom

cover:
  - platform: time_based
    id: livingroom_cover
    name: "Livingroom cover"
    device_class: shutter
    open_duration: 18s
    close_duration: 17s
    has_built_in_endstop: true
    assumed_state: true

    open_action:
      - somfy_rts.open: livingroom_cmd

    close_action:
      - somfy_rts.close: livingroom_cmd

    stop_action:
      - somfy_rts.stop: livingroom_cmd
```

## Component configuration (`somfy_rts:`)

| Variable | Required | Default | Description |
|----------|----------|---------|-------------|
| `id` | Yes | | ID of the Somfy command-layer instance |
| `remote_transmitter` | Yes | | ID of the `remote_transmitter` component |
| `remote_code` | Yes | | Unique 3-byte hex remote code (e.g. `0x6b2a03`) |
| `storage_key` | Yes | | Key for rolling code storage in NVS (max 15 chars, unique per cover) |
| `storage_namespace` | No | `somfy_rts` | NVS namespace for rolling code storage (max 15 chars) |
| `repeat_command_count` | No | `4` | Number of repeated frames after the first frame (1-100) |
| `prog_button` | No | | Optional `button` ID. If set, pressing that button sends `PROG` |

## Actions

These actions can be used in automations, including `time_based` cover actions:

- `somfy_rts.open`
- `somfy_rts.close`
- `somfy_rts.stop`
- `somfy_rts.program`

Examples:

```yaml
on_...:
  then:
    - somfy_rts.open: livingroom_cmd
```

```yaml
on_...:
  then:
    - somfy_rts.program: livingroom_cmd
```

## Generate remote code

The remote code is a 3-byte hex code. Example tool:

- https://www.browserling.com/tools/random-hex

Generate 6 hex digits and prefix with `0x`.

## Pair the cover

Put your cover in program mode with an already paired remote, then send `PROG` from this component (via configured `prog_button` or `somfy_rts.program` action).

## Repeating command setting

Default behavior is to send one initial frame + repeated frames (`repeat_command_count`, default `4`). Some devices require fewer repeats.

## Credits

I originally used the ESPHome custom component from [evgeni](https://github.com/evgeni/esphome-configs/) after finding this article: [Controlling Somfy roller shutters using an ESP32 and ESPHome](https://www.die-welt.net/2021/06/controlling-somfy-roller-shutters-using-an-esp32-and-esphome/).

The later refactoring to use official ESPHome CC1101 + remote components was inspired by [@fawick](https://github.com/fawick/somfy_cover_2025.12).
