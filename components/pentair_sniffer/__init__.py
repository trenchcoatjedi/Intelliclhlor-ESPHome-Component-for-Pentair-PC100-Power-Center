import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, text_sensor, uart
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_POWER,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_WATT,
)

CODEOWNERS = ["@wolfson292"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "text_sensor"]

pentair_sniffer_ns = cg.esphome_ns.namespace("pentair_sniffer")
PentairSniffer = pentair_sniffer_ns.class_(
    "PentairSniffer", cg.Component, uart.UARTDevice
)

CONF_PUMP_ADDRESS = "pump_address"
CONF_LOG_ALL_FRAMES = "log_all_frames"

CONF_VALID_FRAMES = "valid_frames"
CONF_PUMP_FRAMES = "pump_frames"
CONF_CHLORINATOR_FRAMES = "chlorinator_frames"
CONF_CHECKSUM_ERRORS = "checksum_errors"
CONF_PUMP_RPM = "pump_rpm"
CONF_PUMP_WATTS = "pump_watts"
CONF_PUMP_GPM = "pump_gpm"
CONF_LAST_FRAME = "last_frame"

UNIT_RPM = "rpm"
UNIT_GPM = "gpm"
UNIT_FRAMES = "frames"


def _counter_sensor():
    return sensor.sensor_schema(
        unit_of_measurement=UNIT_FRAMES,
        accuracy_decimals=0,
        state_class=STATE_CLASS_TOTAL_INCREASING,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon="mdi:counter",
    )


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PentairSniffer),
            cv.Optional(CONF_PUMP_ADDRESS, default=0x60): cv.hex_uint8_t,
            cv.Optional(CONF_LOG_ALL_FRAMES, default=True): cv.boolean,
            cv.Optional(CONF_VALID_FRAMES): _counter_sensor(),
            cv.Optional(CONF_PUMP_FRAMES): _counter_sensor(),
            cv.Optional(CONF_CHLORINATOR_FRAMES): _counter_sensor(),
            cv.Optional(CONF_CHECKSUM_ERRORS): _counter_sensor(),
            cv.Optional(CONF_PUMP_RPM): sensor.sensor_schema(
                unit_of_measurement=UNIT_RPM,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                icon="mdi:speedometer",
            ),
            cv.Optional(CONF_PUMP_WATTS): sensor.sensor_schema(
                unit_of_measurement=UNIT_WATT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_POWER,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_PUMP_GPM): sensor.sensor_schema(
                unit_of_measurement=UNIT_GPM,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                icon="mdi:water",
            ),
            cv.Optional(CONF_LAST_FRAME): text_sensor.text_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:console",
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    cg.add(var.set_pump_address(config[CONF_PUMP_ADDRESS]))
    cg.add(var.set_log_all(config[CONF_LOG_ALL_FRAMES]))

    sensor_setters = {
        CONF_VALID_FRAMES: var.set_valid_frames_sensor,
        CONF_PUMP_FRAMES: var.set_pump_frames_sensor,
        CONF_CHLORINATOR_FRAMES: var.set_chlorinator_frames_sensor,
        CONF_CHECKSUM_ERRORS: var.set_checksum_errors_sensor,
        CONF_PUMP_RPM: var.set_pump_rpm_sensor,
        CONF_PUMP_WATTS: var.set_pump_watts_sensor,
        CONF_PUMP_GPM: var.set_pump_gpm_sensor,
    }
    for key, setter in sensor_setters.items():
        if key in config:
            cg.add(setter(await sensor.new_sensor(config[key])))

    if CONF_LAST_FRAME in config:
        cg.add(var.set_last_frame_sensor(await text_sensor.new_text_sensor(config[CONF_LAST_FRAME])))
