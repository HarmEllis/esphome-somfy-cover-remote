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
    repeat_command_count: 1
    tilt_repeat_count: 3
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
| `repeat_command_count` | No | `4` | Number of repeated frames after the first frame (0-100). Set to `0` to send only the initial frame. |
| `tilt_repeat_count` | No | `3` | Number of repeated frames for `open_tilt` / `close_tilt` (0-100). Same semantics as `repeat_command_count` (one initial frame + N repeats). Some louvres need a different value; if tilt does not work, sweep `0..4`. |
| `prog_button` | No | | Optional `button` ID. If set, pressing that button sends `PROG` |

## Actions

These actions can be used in automations, including `time_based` cover actions:

- `somfy_rts.open`
- `somfy_rts.close`
- `somfy_rts.stop`
- `somfy_rts.program`
- `somfy_rts.open_tilt`
- `somfy_rts.close_tilt`
- `somfy_rts.send` (generic command — see [Generic send action](#generic-send-action))

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

Default behavior is to send one initial frame + repeated frames (`repeat_command_count`, default `4`). Some devices require fewer repeats, including `repeat_command_count: 0` to send only the initial frame.

## Tilt commands

Somfy RTS louvre covers tilt by sending the same `UP` / `DOWN` command but with a lower number of repeated frames. The `somfy_rts.open_tilt` and `somfy_rts.close_tilt` actions send `UP` / `DOWN` using `tilt_repeat_count` (default `3`) instead of `repeat_command_count`.

The official ESPHome `time_based` cover platform has no tilt hooks, so wire these actions to your own controls. Two common patterns:

### Template buttons (simple)

```yaml
button:
  - platform: template
    name: "Livingroom tilt open"
    on_press:
      - somfy_rts.open_tilt: livingroom_cmd
  - platform: template
    name: "Livingroom tilt close"
    on_press:
      - somfy_rts.close_tilt: livingroom_cmd
```

### Template cover with `tilt_action` (advanced)

```yaml
cover:
  - platform: template
    name: "Livingroom louvre tilt"
    device_class: shutter
    optimistic: true   # no tilt feedback from the motor
    has_position: false
    tilt_action:
      - if:
          condition:
            lambda: 'return tilt > 0.5;'
          then:
            - somfy_rts.open_tilt: livingroom_cmd
          else:
            - somfy_rts.close_tilt: livingroom_cmd
```

If tilt does not work with the default `tilt_repeat_count: 3`, sweep values `0..4` for your louvre model.

## Generic send action

For commands that do not map to the high-level actions — for example "long program" (used to pair a second remote) or experimenting with non-standard repeat counts — use `somfy_rts.send`:

| Field | Required | Default | Description |
|-------|----------|---------|-------------|
| `id` | Yes | | ID of the `somfy_rts` instance |
| `command` | Yes | | One of `UP`, `DOWN`, `MY`, `PROG` |
| `repeat_count` | No | `repeat_command_count` | Templatable integer (0-100). Overrides the configured `repeat_command_count` for this single call. |

Example — long program (15 repeats of `PROG`) to add an extra remote to an already-paired motor:

```yaml
button:
  - platform: template
    name: "Pair extra remote"
    on_press:
      - somfy_rts.send:
          id: livingroom_cmd
          command: PROG
          repeat_count: 15
```

Example — templatable repeat count:

```yaml
on_...:
  then:
    - somfy_rts.send:
        id: livingroom_cmd
        command: UP
        repeat_count: !lambda 'return id(tilt_steps).state;'
```

## Credits

I originally used the ESPHome custom component from [evgeni](https://github.com/evgeni/esphome-configs/) after finding this article: [Controlling Somfy roller shutters using an ESP32 and ESPHome](https://www.die-welt.net/2021/06/controlling-somfy-roller-shutters-using-an-esp32-and-esphome/).

The later refactoring to use official ESPHome CC1101 + remote components was inspired by [@fawick](https://github.com/fawick/somfy_cover_2025.12).
