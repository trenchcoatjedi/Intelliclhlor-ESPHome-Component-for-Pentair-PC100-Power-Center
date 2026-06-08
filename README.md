# IntelliChlor + Pentair Pump — ESPHome External Components

A set of [ESPHome](https://esphome.io/) external components for talking to **Pentair**
pool equipment over RS-485 and exposing it to Home Assistant — without a Pentair
automation panel (EasyTouch / IntelliCenter).

| Component | What it does |
|-----------|--------------|
| [`intellichlor`](#intellichlor) | Read & control a Pentair **IntelliChlor / iChlor** salt chlorine generator (SWG): salt level, water temp, faults, and the chlorine output %. |
| [`pentair_pump`](#pentair_pump) | Master/controller for a Pentair-protocol **variable-speed pump**: read RPM/watts/GPM and set speed, flow, and run state. |
| [`uart_splitter`](#uart_splitter) | Fan a **single** hardware UART out to multiple read-only channels so several components can share one RS-485 bus. |
| [`pentair_sniffer`](#pentair_sniffer) | Passive bus monitor — decodes pump frames and counts traffic for debugging/verification. Writes nothing. |

Each component is a standalone "hub" that you drop into your own ESPHome YAML; every
entity is optional, so you expose only what you want. The ESP can run as a passive
**monitor** of an existing controller, or **take over** and drive the equipment directly.

---

## Hardware

- An **ESP32** (the examples use an `esp32-s3-devkitc-1`, ESP-IDF framework).
- An **RS-485 transceiver** (e.g. MAX485 / a 3.3 V-safe automatic-direction module)
  wired to the equipment's RS-485 / COM bus:
  - ESP `tx_pin` → transceiver `DI`
  - ESP `rx_pin` ← transceiver `RO`
  - Optional `flow_control_pin` → `DE` + `!RE` tied together (for non-auto-direction
    modules in half-duplex).
- The IntelliChlor / Pentair bus is **9600 8N1** (parity `NONE`, 1 stop bit). The
  `intellichlor` component enforces this at validation time.

> ⚠️ **Mains & pool safety.** This involves wiring into pool equipment carrying mains
> voltage. Do the work with power off, and only if you're comfortable with it. This is
> unofficial, reverse-engineered software with **no affiliation with Pentair**. Use at
> your own risk.

---

## Quick start

Point `external_components` at this repo and pull in the components you need:

```yaml
external_components:
  - source: github://wolfson292/intellichlor
    components: [intellichlor]      # add pentair_pump, uart_splitter, pentair_sniffer as needed
    refresh: 1d
```

A complete, copy-paste-friendly single-device config lives in
[`example.yaml`](example.yaml). The sections below summarize each component.

> ESPHome caches external components. If a rebuild doesn't pick up changes, lower
> `refresh` or clear the `.esphome/` directory so it re-pulls.

---

## intellichlor

Talks to the IntelliChlor cell (a slave at bus address `0x50`). With **takeover off**
the ESP passively reads the cell as the existing controller polls it; with **takeover
on** the ESP impersonates the controller and asserts the output % itself.

```yaml
uart:
  id: swg_uart
  tx_pin: GPIO17
  rx_pin: GPIO18
  baud_rate: 9600
  parity: NONE        # required
  stop_bits: 1        # required

# Optional: a time source lets "boost" survive a reboot/OTA (stored as an
# absolute timestamp in flash).
time:
  - platform: sntp
    id: sntp_time

intellichlor:
  id: swg
  uart_id: swg_uart
  # flow_control_pin: GPIO8   # optional DE/!RE pin for half-duplex transceivers
  time_id: sntp_time          # optional, enables boost persistence
  update_interval: 60s
```

### Entities

**Sensors**

| Key | Description |
|-----|-------------|
| `salt_ppm` | Salt level, ppm |
| `water_temp` | Water temperature (**reported in °F**) |
| `output_percent` | Cell-reported *actual* generation % |
| `set_percent` | Last commanded output % |
| `boost_remaining` | Minutes left in an active boost |
| `status` | Status byte from the cell |
| `error` | Raw alarm/fault bitmask (decode manually if needed) |

**Binary sensors (fault bits):** `no_flow`, `low_salt`, `very_low_salt`,
`high_current`, `clean`, `low_volts`, `low_temp`, `check_pcb`.

**Text sensors:** `version` (model string, e.g. `Intellichlor--40`),
`firmware_version` (cell firmware, e.g. `1.8`).

**Controls**

| Platform | Key | Description |
|----------|-----|-------------|
| `number` | `swg_percent` | Output setpoint, 0–100% (persisted across reboot/OTA) |
| `switch` | `takeover_mode` | ON = ESP drives the cell; OFF = passive monitor |
| `select` | `swg_boost` | Run at 100% for a fixed time: `Off / 6h / 12h / 24h / 48h` |
| `button` | `end_boost` | Cancel an active boost early |

> **Boost only takes effect while `takeover_mode` is ON.** With takeover off the cell
> is driven by the real controller; boost will log a warning and do nothing.

> **Fault bit caveat:** the `high_current` / `clean` bits follow the protocol spec,
> which diverges from one hardware capture. If a real "clean cell" condition lights up
> "High Current" (or vice versa), see [`AGENTS.md`](AGENTS.md) — the raw `error` sensor
> always carries the unmodified value.

See the [example](example.yaml) for the full entity block.

---

## pentair_pump

Master/controller for a Pentair-protocol variable-speed pump (e.g. a LINGXIAO VS pump
in ECOM mode) at bus address `0x60`. Polls the pump, can hold it in remote control, and
sets speed/flow + run state.

```yaml
pentair_pump:
  id: pump
  uart_id: pump_uart       # its own bus, or a uart_splitter channel
  address: 0x60            # pump address (default 0x60)
  source_address: 0x10     # controller/source address (default 0x10)
  update_interval: 2s
```

### Entities

**Sensors:** `rpm`, `watts`, `gpm`, `mode`, `drive_state`, `run_state`, `status_code`.
**Binary sensor:** `running`.
**Text sensor:** `last_status` (full hex of the last status reply).

**Controls**

| Platform | Key | Description |
|----------|-----|-------------|
| `switch` | `takeover` | Master enable: actively control vs monitor-only |
| `switch` | `run` | Start/stop the pump |
| `select` | `mode` | Setpoint mode: `Speed (RPM)` or `Flow (GPM)` |
| `number` | `target_rpm` | Target speed, 600–3450 RPM (step 10) |
| `number` | `target_gpm` | Target flow, 15–130 GPM (step 1) |

---

## uart_splitter

The IntelliChlor and the pump can share the same physical RS-485 bus. ESPHome's `uart`
hub is single-owner, so this component takes over one hardware UART and fans it out to
several **channels** that each look like a UART to downstream components.

```yaml
uart:
  id: mod_uart
  tx_pin: GPIO17
  rx_pin: GPIO18
  baud_rate: 9600

uart_splitter:
  uart_id: mod_uart
  channels:
    - id: uart_chlor       # -> intellichlor
    - id: uart_pump        # -> pentair_pump
    - id: uart_pump_sniff  # -> pentair_sniffer

intellichlor:
  uart_id: uart_chlor
  # ...
pentair_pump:
  uart_id: uart_pump
  # ...
```

Channels default to 9600 8N1; override `baud_rate` / `data_bits` / `stop_bits` /
`parity` per channel if needed. Components on shared channels queue and gate their
transmits (≈one frame per 100 ms) to stay polite on the shared bus.

---

## pentair_sniffer

A passive, write-nothing monitor of the pump protocol — useful for verifying what the
pump masters are actually putting on the wire. Decodes frames and exposes counters.

```yaml
pentair_sniffer:
  id: sniffer
  uart_id: uart_pump_sniff
  pump_address: 0x60
  log_all_frames: true
  # all sensors below are optional:
  pump_rpm:           { name: "Sniffer Pump RPM" }
  pump_watts:         { name: "Sniffer Pump Watts" }
  pump_gpm:           { name: "Sniffer Pump GPM" }
  valid_frames:       { name: "Sniffer Valid Frames" }
  pump_frames:        { name: "Sniffer Pump Frames" }
  chlorinator_frames: { name: "Sniffer Chlorinator Frames" }
  checksum_errors:    { name: "Sniffer Checksum Errors" }
  last_frame:         { name: "Sniffer Last Frame" }
```

---

## Building & testing

There's no standalone firmware here — these are components compiled by ESPHome inside a
consuming project. To compile-check the whole set against this clone:

```bash
esphome config  compile-test.yaml
esphome compile compile-test.yaml
```

[`compile-test.yaml`](compile-test.yaml) wires all four components together exactly as
on the live board (single bus → `uart_splitter` → chlorinator + pump + sniffer) and is
a good reference for a combined setup.

---

## Protocol

The reverse-engineered RS-485 wire format (framing, DLE byte-stuffing, checksum, command
set, and status decoding) is documented in
[`docs/IntelliChlor_Protocol.md`](docs/IntelliChlor_Protocol.md). Implementation notes
and history for component authors live in [`AGENTS.md`](AGENTS.md).

---

## License

[GNU General Public License v3.0](LICENSE).

Not affiliated with or endorsed by Pentair. "IntelliChlor", "IntelliCenter", and
"EasyTouch" are trademarks of their respective owners.
