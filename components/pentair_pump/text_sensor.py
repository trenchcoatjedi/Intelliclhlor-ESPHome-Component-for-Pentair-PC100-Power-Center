import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import ENTITY_CATEGORY_DIAGNOSTIC

from . import CONF_PENTAIR_PUMP_ID, PentairPump

DEPENDENCIES = ["pentair_pump"]

CONF_LAST_STATUS = "last_status"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PENTAIR_PUMP_ID): cv.use_id(PentairPump),
        cv.Optional(CONF_LAST_STATUS): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:console",
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_PENTAIR_PUMP_ID])
    if CONF_LAST_STATUS in config:
        cg.add(
            hub.set_last_status_text_sensor(
                await text_sensor.new_text_sensor(config[CONF_LAST_STATUS])
            )
        )
