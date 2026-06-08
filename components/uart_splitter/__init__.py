import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.components.const import CONF_DATA_BITS, CONF_PARITY, CONF_STOP_BITS
from esphome.const import CONF_BAUD_RATE, CONF_CHANNELS, CONF_ID, CONF_UART_ID

CODEOWNERS = ["@wolfson292"]
DEPENDENCIES = ["uart"]

uart_splitter_ns = cg.esphome_ns.namespace("uart_splitter")
UARTSplitter = uart_splitter_ns.class_("UARTSplitter", cg.Component)
UARTSplitterChannel = uart_splitter_ns.class_("UARTSplitterChannel", uart.UARTComponent)

# Each channel advertises UART settings so that downstream uart devices (which
# final-validate the bus they attach to) see a hub-like config.  Defaults match a
# 9600 8N1 RS485 bus; keep them in sync with the hardware uart this splits.
CHANNEL_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(UARTSplitterChannel),
        cv.Optional(CONF_BAUD_RATE, default=9600): cv.int_range(min=1),
        cv.Optional(CONF_STOP_BITS, default=1): cv.one_of(1, 2, int=True),
        cv.Optional(CONF_DATA_BITS, default=8): cv.int_range(min=5, max=8),
        cv.Optional(CONF_PARITY, default="NONE"): cv.enum(
            uart.UART_PARITY_OPTIONS, upper=True
        ),
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(UARTSplitter),
        # The single real hardware UART this splitter takes over.
        cv.Required(CONF_UART_ID): cv.use_id(uart.UARTComponent),
        cv.Required(CONF_CHANNELS): cv.All(
            cv.ensure_list(CHANNEL_SCHEMA), cv.Length(min=1)
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    parent = await cg.get_variable(config[CONF_UART_ID])
    cg.add(var.set_parent(parent))

    for ch in config[CONF_CHANNELS]:
        chan = cg.new_Pvariable(ch[CONF_ID])
        cg.add(chan.set_parent(parent))
        cg.add(chan.set_baud_rate(ch[CONF_BAUD_RATE]))
        cg.add(chan.set_stop_bits(ch[CONF_STOP_BITS]))
        cg.add(chan.set_data_bits(ch[CONF_DATA_BITS]))
        cg.add(chan.set_parity(ch[CONF_PARITY]))
        cg.add(var.add_channel(chan))
