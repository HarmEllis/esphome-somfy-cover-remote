# ESPHome Somfy cover remote
This is an external component for [ESPHome](https://esphome.io/), to control Somfy RTS covers from for example [Home Assistant](https://www.home-assistant.io/).

Uses the official ESPHome [CC1101](https://esphome.io/components/cc1101/) and [Remote Transmitter](https://esphome.io/components/remote_transmitter/) components for RF transmission. Works with both the Arduino and ESP-IDF frameworks.

## Required hardware
- ESP32
- CC1101 RF module

## Setup
Use the following ESPHome yaml as a base for your Somfy controller. Add one or more covers, depending on your needs.

```yaml
esphome:
  name: somfycontroller

external_components:
  - source: github://HarmEllis/esphome-somfy-cover-remote@main
    components: [ somfy_cover ]

esp32:
  board: esp32dev
  framework:
    type: esp-idf

# Enable logging
logger:

# Enable Home Assistant API
api:

ota:
  platform: esphome
  password: !secret ota_password

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

  # Enable fallback hotspot (captive portal) in case wifi connection fails
  ap:
    ssid: "Somfycontroller Fallback Hotspot"
    password: !secret fallback_hotspot

button:
  - platform: template
    id: "program_livingroom"
    name: "Program livingroom"
  - platform: template
    id: "program_kitchen"
    name: "Program kitchen"
  - platform: template
    id: "program_study"
    name: "Program study"
  - platform: template
    id: "program_bathroom"
    name: "Program bathroom"

# SPI bus for CC1101 (adjust pins for your board)
spi:
  clk_pin: GPIO14
  mosi_pin: GPIO12
  miso_pin: GPIO39

# Official ESPHome CC1101 component
cc1101:
  cs_pin: GPIO15
  frequency: 433.42MHz

# Remote transmitter connected to CC1101 GDO0 pin
remote_transmitter:
  id: "transmitter"
  pin: GPIO2
  carrier_duty_percent: 100%
  on_transmit:
    then:
      - cc1101.begin_tx
  on_complete:
    then:
      - cc1101.set_idle

cover:
  - platform: somfy_cover
    id: "livingroom"
    name: "Livingroom cover"
    device_class: shutter
    open_duration: 18s
    close_duration: 17s
    remote_code: 0x6b2a03
    storage_key: "livingroom"
    prog_button: "program_livingroom"
    remote_transmitter: "transmitter"

  - platform: somfy_cover
    id: "kitchen"
    name: "Kitchen cover"
    device_class: shutter
    open_duration: 26s
    close_duration: 25s
    remote_code: 0x0bf93b
    storage_key: "kitchen"
    prog_button: "program_kitchen"
    remote_transmitter: "transmitter"

  - platform: somfy_cover
    id: "study"
    name: "Study cover"
    device_class: shutter
    open_duration: 18s
    close_duration: 17s
    remote_code: 0x09a1c3
    storage_key: "study"
    prog_button: "program_study"
    remote_transmitter: "transmitter"

  - platform: somfy_cover
    id: "bathroom"
    name: "Bathroom Cover"
    device_class: shutter
    open_duration: 18s
    close_duration: 17s
    remote_code: 0x449677
    storage_key: "bathroom"
    prog_button: "program_bathroom"
    remote_transmitter: "transmitter"
    repeat_command_count: 1
```

## Configuration variables

| Variable | Required | Default | Description |
|----------|----------|---------|-------------|
| `remote_transmitter` | Yes | | ID of the `remote_transmitter` component |
| `prog_button` | Yes | | ID of a `button` component used for pairing |
| `open_duration` | Yes | | Time for the cover to fully open |
| `close_duration` | Yes | | Time for the cover to fully close |
| `remote_code` | Yes | | Unique 3-byte hex remote code (e.g. `0x6b2a03`) |
| `storage_key` | Yes | | Key for rolling code storage in NVS (max 15 chars, unique per cover) |
| `storage_namespace` | No | `somfy_cover` | NVS namespace for rolling code storage (max 15 chars) |
| `repeat_command_count` | No | `4` | Number of times to repeat the RF command (1-100) |

## Generate remote code
The remote code is a three byte hex code.
For example, use the website: https://www.browserling.com/tools/random-hex
Set to 6 digits and add `0x` in front of the generated hex number.

## Pair the cover
Put your cover in program mode with another remote, then use the `Program x` button to pair with the ESP. From then on the cover should respond to the ESPHome Somfy controller. You can also connect multiple covers by pairing them one by one with the same `Program x` button.

## Repeating command setting
The default is to send a command four times. Some devices do not handle this well and should only receive the command one time. For these devices the optional parameter `repeat_command_count` can be set in the yaml for the cover.

## Migrating from the old version

If you are upgrading from the version that used `esphome-cc1101` (the custom CC1101 component), you need to make the following changes to your YAML:

1. **Remove** the `esphome-cc1101` external component reference
2. **Add** the `spi`, `cc1101` (official), and `remote_transmitter` components (see example above)
3. **Replace** `cc1101_module` with `remote_transmitter` in each cover configuration

**Note:** If you are also switching from the Arduino to the ESP-IDF framework, the NVS partition will be erased. This means your rolling codes will be lost and you will need to generate new `remote_code` values and re-pair your covers.

## Credits
I originally used the ESPHome custom component from [evgeni](https://github.com/evgeni/esphome-configs/) after I found his article [Controlling Somfy roller shutters using an ESP32 and ESPHome](https://www.die-welt.net/2021/06/controlling-somfy-roller-shutters-using-an-esp32-and-esphome/).

But I wanted to be able to specify the cover position by used the ESPHome [Time Based Cover](https://esphome.io/components/cover/time_based.html) component. I used the code from evgeni as a base and used the ESPHome Custom component option to implement this. Since the Custom component option is deprecated and removed, I needed another solution. Therefore I created this external component.

The refactoring to use the official ESPHome CC1101 and Remote Transmitter components was inspired by the proof-of-concept from [@fawick](https://github.com/fawick/somfy_cover_2025.12).
