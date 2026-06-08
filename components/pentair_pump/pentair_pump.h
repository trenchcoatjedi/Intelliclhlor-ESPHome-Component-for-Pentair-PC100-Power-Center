#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/number/number.h"
#include "esphome/components/switch/switch.h"

#include <queue>
#include <vector>

namespace esphome {
namespace pentair_pump {

// Master/controller for a Pentair-protocol variable-speed pump (e.g. the LINGXIAO
// VS pump in ECOM mode) at address 0x60.  Polls the pump, holds it in remote
// control, sets RPM/GPM + run state, and decodes its status replies.
//
// Frame: FF 00 FF  A5 00 <dst> <src> <cmd> <len> <data...> <ckH> <ckL>, checksum
// = 16-bit sum of A5..last-data, big-endian.  Shares one RS485 bus with the
// IntelliChlor via uart_splitter, so transmits are queued and gated (one frame
// per ~100 ms) to stay a polite bus citizen.
class PentairPump : public PollingComponent, public uart::UARTDevice {
  SUB_SENSOR(rpm)
  SUB_SENSOR(watts)
  SUB_SENSOR(gpm)
  SUB_BINARY_SENSOR(running)
  SUB_SWITCH(run)
  SUB_NUMBER(target_rpm)
  SUB_NUMBER(target_gpm)

 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_address(uint8_t a) { this->address_ = a; }
  void set_source_address(uint8_t a) { this->source_ = a; }

  // Called by the sub-entities (number control / switch write_state).
  void set_target_rpm(float rpm);
  void set_target_gpm(float gpm);
  void set_run_state(bool run);

 protected:
  void enqueue_frame_(uint8_t cmd, const std::vector<uint8_t> &data);
  void parse_buffer_();
  void handle_frame_(size_t total);

  uint8_t address_{0x60};
  uint8_t source_{0x10};
  uint16_t target_rpm_{1500};
  uint8_t target_gpm_{0};
  bool gpm_mode_{false};
  bool run_{false};

  std::vector<uint8_t> buf_;
  std::queue<std::vector<uint8_t>> send_queue_;
  uint32_t last_send_{0};
};

// Writable number that pushes its value back into the hub.  kind: 0 = RPM, 1 = GPM.
class PentairPumpNumber : public number::Number {
 public:
  void set_parent(PentairPump *parent) { this->parent_ = parent; }
  void set_kind(uint8_t kind) { this->kind_ = kind; }

 protected:
  void control(float value) override;
  PentairPump *parent_{nullptr};
  uint8_t kind_{0};
};

// Run/stop switch that drives the hub.
class PentairPumpRunSwitch : public switch_::Switch {
 public:
  void set_parent(PentairPump *parent) { this->parent_ = parent; }

 protected:
  void write_state(bool state) override;
  PentairPump *parent_{nullptr};
};

}  // namespace pentair_pump
}  // namespace esphome
