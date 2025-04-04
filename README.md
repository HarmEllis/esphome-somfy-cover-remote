# ESPHome Somfy cover remote
This is an external component for [ESPHOME](https://esphome.io/), to control Somfy RTS covers from for example [Home Assisant](https://www.home-assistant.io/).

## Required hardware
- ESP32
- CC1101 RF module

## Setup
Use the following ESPHome yaml as a base for your Somfy controller. Add one ore more covers, depending on your needs.

```
esphome:
  name: somfycontroller

external_components:
  - source: github://HarmEllis/esphome-somfy-cover-remote@main
    components: [ somfy_cover ]

esp32:
  board: esp32dev
  framework:
    type: arduino

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

  Enable fallback hotspot (captive portal) in case wifi connection fails
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

cover:
  - platform: somfy_cover
    id: "livingroom"
    name: "Livingroom cover"
    device_class: shutter
    open_duration: 18s
    close_duration: 17s
    cover_remote_code: 0x6b2a03
    cover_prog_button: "program_livingroom"
  
  - platform: somfy_cover
    id: "kitchen"
    name: "Kitchen cover"
    device_class: shutter
    open_duration: 26s
    close_duration: 25s
    cover_remote_code: 0x0bf93b
    cover_prog_button: "program_kitchen"
  
  - platform: somfy_cover
    id: "study"
    name: "Study cover"
    device_class: shutter
    open_duration: 18s
    close_duration: 17s
    cover_remote_code: 0x09a1c3
    cover_prog_button: "program_study"

  - platform: somfy_cover
    id: "bathroom"
    name: "Bathroom Cover"
    device_class: shutter
    open_duration: 18s
    close_duration: 17s
    cover_remote_code: 0x449677
    cover_prog_button: "program_bathroom"

```

## Generate remote code 
The remote code is a three byte hex code.
For example, use the website: https://www.browserling.com/tools/random-hex  
Set to 6 digits and add `0x` in front of the generated hex number.

## CC1101 module connection
Connect the following CC1101 pints to these GPIO pins:
- EMITTER_GPIO 2
- SCK_PIN 14
- MISO_PIN 39
- MOSI_PIN 12
- SS_PIN 15
- GDO2 4
- VCC 3.3v
- GND GND

## Pair the cover
Put your cover in program mode with another remote, then use the `Program x` button to pair with the ESP. From then on the cover should respond to the ESPHome Somfy controller.

## Credits
I originally used the ESPHome custom component from [evgeni](https://github.com/evgeni/esphome-configs/) after I found his article [Controlling Somfy roller shutters using an ESP32 and ESPHome](https://www.die-welt.net/2021/06/controlling-somfy-roller-shutters-using-an-esp32-and-esphome/).

But I wanted to be able to specify the cover position by used the ESPHome [Time Based Cover](https://esphome.io/components/cover/time_based.html) component. I used the code from evgeni as as base and used the ESPHome Custom component option to implement this. Since the Custom component option is deprecated and removed, I needed another solution. Therefore I created this external component.
