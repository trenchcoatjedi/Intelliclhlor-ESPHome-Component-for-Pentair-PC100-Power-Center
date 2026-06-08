import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch

from . import CONF_PENTAIR_PUMP_ID, PentairPump, pentair_pump_ns

DEPENDENCIES = ["pentair_pump"]

PentairPumpRunSwitch = pentair_pump_ns.class_("PentairPumpRunSwitch", switch.Switch)

CONF_RUN = "run"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PENTAIR_PUMP_ID): cv.use_id(PentairPump),
        cv.Optional(CONF_RUN): switch.switch_schema(
            PentairPumpRunSwitch, icon="mdi:pump"
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_PENTAIR_PUMP_ID])
    if CONF_RUN in config:
        sw = await switch.new_switch(config[CONF_RUN])
        cg.add(sw.set_parent(hub))
        cg.add(hub.set_run_switch(sw))
