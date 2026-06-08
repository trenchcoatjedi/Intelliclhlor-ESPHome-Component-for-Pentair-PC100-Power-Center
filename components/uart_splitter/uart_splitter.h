#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

#include <vector>

namespace esphome {
namespace uart_splitter {

// One virtual UART presented to a downstream component.  It looks like a normal
// uart::UARTComponent (so any UARTDevice — the IntelliChlor component, the
// pentair_pump master, the pentair_sniffer, etc. — can attach via `uart_id:`),
// but its RX comes from a private buffer the splitter fills, and its TX is
// forwarded to the single real hardware UART.  Multiple channels each get a full
// copy of every received byte, which sidesteps ESPHome's destructive
// single-reader UART model.
class UARTSplitterChannel : public uart::UARTComponent {
 public:
  void set_parent(uart::UARTComponent *parent) { this->parent_ = parent; }

  // ---- uart::UARTComponent interface ----
  void write_array(const uint8_t *data, size_t len) override { this->parent_->write_array(data, len); }
  bool peek_byte(uint8_t *data) override {
    if (this->rx_.empty())
      return false;
    *data = this->rx_.front();
    return true;
  }
  bool read_array(uint8_t *data, size_t len) override {
    if (this->rx_.size() < len)
      return false;
    for (size_t i = 0; i < len; i++)
      data[i] = this->rx_[i];
    this->rx_.erase(this->rx_.begin(), this->rx_.begin() + len);
    return true;
  }
  size_t available() override { return this->rx_.size(); }
  uart::UARTFlushResult flush() override { return this->parent_->flush(); }

  // Called by the splitter for every byte read from the hardware UART.
  void push_rx(uint8_t b) {
    this->rx_.push_back(b);
    if (this->rx_.size() > MAX_CH_BUF)
      this->rx_.erase(this->rx_.begin());  // overflow guard: drop oldest
  }

 protected:
  void check_logger_conflict() override {}
  static const size_t MAX_CH_BUF = 512;
  uart::UARTComponent *parent_{nullptr};
  std::vector<uint8_t> rx_;
};

// Owns the single hardware UART and fans its RX stream out to N channels.
class UARTSplitter : public Component {
 public:
  void set_parent(uart::UARTComponent *parent) { this->parent_ = parent; }
  void add_channel(UARTSplitterChannel *ch) { this->channels_.push_back(ch); }

  void setup() override;
  void loop() override;
  void dump_config() override;
  // Drain the bus early each loop, before downstream consumers run.
  float get_setup_priority() const override { return setup_priority::BUS; }

 protected:
  uart::UARTComponent *parent_{nullptr};
  std::vector<UARTSplitterChannel *> channels_;
};

}  // namespace uart_splitter
}  // namespace esphome
