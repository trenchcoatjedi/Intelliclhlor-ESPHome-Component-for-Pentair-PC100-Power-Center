#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include <vector>

namespace esphome {
namespace pentair_sniffer {

// Passive RS485 sniffer/decoder for the Pentair pump protocol (IntelliFlo /
// IntelliCom framing: FF 00 FF A5 ver dst src cmd len data[] ckH ckL).
//
// It is RECEIVE-ONLY: it never drives the bus, so it can listen alongside an
// active IntelliChlor controller and the pump master without colliding.  It also
// counts (but does not decode) IntelliChlor frames (10 02 .. 10 03) so you can
// confirm every device shares the wire.
class PentairSniffer : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  // Run after UART is up, before sensors expect data.
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_pump_address(uint8_t a) { this->pump_address_ = a; }
  void set_log_all(bool b) { this->log_all_ = b; }

  void set_valid_frames_sensor(sensor::Sensor *s) { this->valid_frames_sensor_ = s; }
  void set_pump_frames_sensor(sensor::Sensor *s) { this->pump_frames_sensor_ = s; }
  void set_chlorinator_frames_sensor(sensor::Sensor *s) { this->chlorinator_frames_sensor_ = s; }
  void set_checksum_errors_sensor(sensor::Sensor *s) { this->checksum_errors_sensor_ = s; }
  void set_pump_rpm_sensor(sensor::Sensor *s) { this->pump_rpm_sensor_ = s; }
  void set_pump_watts_sensor(sensor::Sensor *s) { this->pump_watts_sensor_ = s; }
  void set_pump_gpm_sensor(sensor::Sensor *s) { this->pump_gpm_sensor_ = s; }
  void set_last_frame_sensor(text_sensor::TextSensor *s) { this->last_frame_sensor_ = s; }

 protected:
  enum ParseResult { PR_OK, PR_NEED_MORE, PR_BAD };
  ParseResult parse_pentair_();
  ParseResult parse_chlorinator_();
  void handle_pentair_(size_t total);
  void publish_stats_();

  std::vector<uint8_t> buf_;
  uint8_t pump_address_{0x60};
  bool log_all_{true};

  uint32_t cnt_valid_{0};
  uint32_t cnt_pump_{0};
  uint32_t cnt_chlor_{0};
  uint32_t cnt_cksum_err_{0};
  bool stats_dirty_{false};
  uint32_t last_pub_{0};

  sensor::Sensor *valid_frames_sensor_{nullptr};
  sensor::Sensor *pump_frames_sensor_{nullptr};
  sensor::Sensor *chlorinator_frames_sensor_{nullptr};
  sensor::Sensor *checksum_errors_sensor_{nullptr};
  sensor::Sensor *pump_rpm_sensor_{nullptr};
  sensor::Sensor *pump_watts_sensor_{nullptr};
  sensor::Sensor *pump_gpm_sensor_{nullptr};
  text_sensor::TextSensor *last_frame_sensor_{nullptr};
};

}  // namespace pentair_sniffer
}  // namespace esphome
