import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    DEVICE_CLASS_POWER,
    STATE_CLASS_MEASUREMENT,
    UNIT_WATT,
)

from . import CONF_PENTAIR_PUMP_ID, PentairPump

DEPENDENCIES = ["pentair_pump"]

CONF_RPM = "rpm"
CONF_WATTS = "watts"
CONF_GPM = "gpm"

UNIT_RPM = "rpm"
UNIT_GPM = "gpm"

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
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_PENTAIR_PUMP_ID])
    if CONF_RPM in config:
        cg.add(hub.set_rpm_sensor(await sensor.new_sensor(config[CONF_RPM])))
    if CONF_WATTS in config:
        cg.add(hub.set_watts_sensor(await sensor.new_sensor(config[CONF_WATTS])))
    if CONF_GPM in config:
        cg.add(hub.set_gpm_sensor(await sensor.new_sensor(config[CONF_GPM])))
