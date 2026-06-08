import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number

from . import CONF_PENTAIR_PUMP_ID, PentairPump, pentair_pump_ns

DEPENDENCIES = ["pentair_pump"]

PentairPumpNumber = pentair_pump_ns.class_("PentairPumpNumber", number.Number)

CONF_TARGET_RPM = "target_rpm"
CONF_TARGET_GPM = "target_gpm"

# Pentair VS pumps run ~600-3450 RPM; flow models report ~15-130 GPM.
RPM_MIN, RPM_MAX, RPM_STEP = 600, 3450, 10
GPM_MIN, GPM_MAX, GPM_STEP = 15, 130, 1

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PENTAIR_PUMP_ID): cv.use_id(PentairPump),
        cv.Optional(CONF_TARGET_RPM): number.number_schema(
            PentairPumpNumber, icon="mdi:speedometer", unit_of_measurement="rpm"
        ),
        cv.Optional(CONF_TARGET_GPM): number.number_schema(
            PentairPumpNumber, icon="mdi:water", unit_of_measurement="gpm"
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_PENTAIR_PUMP_ID])
    if CONF_TARGET_RPM in config:
        n = await number.new_number(
            config[CONF_TARGET_RPM],
            min_value=RPM_MIN,
            max_value=RPM_MAX,
            step=RPM_STEP,
        )
        cg.add(n.set_parent(hub))
        cg.add(n.set_kind(0))
        cg.add(hub.set_target_rpm_number(n))
    if CONF_TARGET_GPM in config:
        n = await number.new_number(
            config[CONF_TARGET_GPM],
            min_value=GPM_MIN,
            max_value=GPM_MAX,
            step=GPM_STEP,
        )
        cg.add(n.set_parent(hub))
        cg.add(n.set_kind(1))
        cg.add(hub.set_target_gpm_number(n))
