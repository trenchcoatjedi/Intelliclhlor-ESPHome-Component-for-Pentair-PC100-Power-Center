import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    DEVICE_CLASS_POWER,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_WATT,
)

from . import CONF_PENTAIR_PUMP_ID, PentairPump

DEPENDENCIES = ["pentair_pump"]

CONF_RPM = "rpm"
CONF_WATTS = "watts"
CONF_GPM = "gpm"
CONF_MODE = "mode"
CONF_DRIVE_STATE = "drive_state"
CONF_RUN_STATE = "run_state"
CONF_STATUS_CODE = "status_code"

UNIT_RPM = "rpm"
UNIT_GPM = "gpm"


def _raw_byte_sensor(icon):
    # Raw status-reply bytes, exposed as diagnostics.
    return sensor.sensor_schema(
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon=icon,
    )


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PENTAIR_PUMP_ID): cv.use_id(PentairPump),
        cv.Optional(CONF_RPM): sensor.sensor_schema(
            unit_of_measurement=UNIT_RPM,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:speedometer",
        ),
        cv.Optional(CONF_WATTS): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_GPM): sensor.sensor_schema(
            unit_of_measurement=UNIT_GPM,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:water",
        ),
        cv.Optional(CONF_MODE): _raw_byte_sensor("mdi:cog"),
        cv.Optional(CONF_DRIVE_STATE): _raw_byte_sensor("mdi:engine"),
        cv.Optional(CONF_RUN_STATE): _raw_byte_sensor("mdi:play-pause"),
        cv.Optional(CONF_STATUS_CODE): _raw_byte_sensor("mdi:alert-circle"),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_PENTAIR_PUMP_ID])
    setters = {
        CONF_RPM: hub.set_rpm_sensor,
        CONF_WATTS: hub.set_watts_sensor,
        CONF_GPM: hub.set_gpm_sensor,
        CONF_MODE: hub.set_mode_sensor,
        CONF_DRIVE_STATE: hub.set_drive_state_sensor,
        CONF_RUN_STATE: hub.set_run_state_sensor,
        CONF_STATUS_CODE: hub.set_status_code_sensor,
    }
    for key, setter in setters.items():
        if key in config:
            cg.add(setter(await sensor.new_sensor(config[key])))
