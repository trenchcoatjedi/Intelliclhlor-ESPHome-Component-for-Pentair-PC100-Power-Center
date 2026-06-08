import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import DEVICE_CLASS_RUNNING

from . import CONF_PENTAIR_PUMP_ID, PentairPump

DEPENDENCIES = ["pentair_pump"]

CONF_RUNNING = "running"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PENTAIR_PUMP_ID): cv.use_id(PentairPump),
        cv.Optional(CONF_RUNNING): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_RUNNING,
            icon="mdi:pump",
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_PENTAIR_PUMP_ID])
    if CONF_RUNNING in config:
        cg.add(
            hub.set_running_binary_sensor(
                await binary_sensor.new_binary_sensor(config[CONF_RUNNING])
            )
        )
