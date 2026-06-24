# Migration Guide: `cover.platform=somfy_cover` -> `somfy_rts` + `time_based`

This repository switched from a custom cover platform to a command-layer component.

Old architecture:

- `cover:`
  - `platform: somfy_cover`
  - inherited internal `TimeBasedCover` implementation

New architecture:

- `somfy_rts:` defines Somfy RTS command senders (rolling code + RF frame transmission)
- `cover: - platform: time_based` handles position estimation and Home Assistant cover behavior
- `open_action` / `close_action` / `stop_action` call `somfy_rts.*` actions

## Why this change

- Removes coupling to internal ESPHome `TimeBasedCover` C++ implementation details.
- Uses the standard, documented `time_based` cover platform for position logic.
- Makes the Somfy part a focused RF command layer.

## Breaking change

`cover: - platform: somfy_cover` is no longer supported.

You must migrate to the new YAML structure below.

## Config mapping

Old `cover.platform=somfy_cover` fields -> New location

- `id` -> `somfy_rts.id`
- `remote_transmitter` -> `somfy_rts.remote_transmitter`
- `remote_code` -> `somfy_rts.remote_code`
- `storage_key` -> `somfy_rts.storage_key`
- `storage_namespace` -> `somfy_rts.storage_namespace`
- `repeat_command_count` -> `somfy_rts.repeat_command_count`
- `tilt_repeat_count` -> `somfy_rts.tilt_repeat_count`
- `prog_button` -> optional `somfy_rts.prog_button` (or call `somfy_rts.program` in automations)
- `open_duration` -> `cover.platform=time_based.open_duration`
- `close_duration` -> `cover.platform=time_based.close_duration`

## Important: keep legacy storage namespace during migration

The new component default is `storage_namespace: "somfy_rts"`. For existing installations, set:

- `storage_namespace: "somfy_cover"`

Reason:

- Rolling codes are persisted in NVS under `storage_namespace + storage_key`.
- If you switch namespace immediately, the device starts from a different rolling-code counter.
- Somfy motors expect a strictly increasing rolling code from the paired remote identity, so this can cause commands to stop working until re-synced/repaired.

## Minimal migration example

```yaml
somfy_rts:
  - id: livingroom_cmd
    remote_transmitter: transmitter
    remote_code: 0x6b2a03
    storage_key: "livingroom"
    storage_namespace: "somfy_cover"
    repeat_command_count: 1
    tilt_repeat_count: 3

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

## Pairing / PROG

Option A (attach existing template button):

```yaml
button:
  - platform: template
    id: program_livingroom
    name: "Program livingroom"

somfy_rts:
  - id: livingroom_cmd
    # ...
    prog_button: program_livingroom
```

Option B (manual automation action):

```yaml
button:
  - platform: template
    name: "Program livingroom"
    on_press:
      - somfy_rts.program: livingroom_cmd
```

## Notes

- Keep `remote_transmitter.carrier_duty_percent: 100%` for RF.
- Rolling codes are still persisted in NVS per `storage_namespace` + `storage_key`.
