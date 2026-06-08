#include "uart_splitter.h"
#include "esphome/core/log.h"

namespace esphome {
namespace uart_splitter {

static const char *const TAG = "uart_splitter";

void UARTSplitter::setup() {
  // Channel framing (baud/parity/stop/data) is set from config at codegen time.
  ESP_LOGCONFIG(TAG, "UART splitter ready: 1 hardware port -> %u channel(s)",
                (unsigned) this->channels_.size());
}

void UARTSplitter::loop() {
  // Drain the hardware UART once per loop and copy every byte to all channels.
  // Each channel keeps its own read cursor, so the IntelliChlor decoder and the
  // pump master/sniffer all see the complete, identical bus stream.
  uint8_t b;
  while (this->parent_->available()) {
    if (!this->parent_->read_byte(&b))
      break;
    for (auto *ch : this->channels_)
      ch->push_rx(b);
  }
}

void UARTSplitter::dump_config() {
  ESP_LOGCONFIG(TAG, "UART Splitter:");
  ESP_LOGCONFIG(TAG, "  Virtual channels: %u", (unsigned) this->channels_.size());
}

}  // namespace uart_splitter
}  // namespace esphome
