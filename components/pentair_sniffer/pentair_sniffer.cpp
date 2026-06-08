#include "pentair_sniffer.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace pentair_sniffer {

static const char *const TAG = "pentair_sniffer";

// Pentair frames are small; anything claiming a longer payload is noise.
static const uint8_t MAX_PENTAIR_LEN = 64;
// IntelliChlor frames are short; bound the search for the 10 03 terminator.
static const size_t MAX_CHLOR_FRAME = 32;
// Cap the working buffer so a flood of noise can't grow it without bound.
static const size_t MAX_BUF = 384;

void PentairSniffer::setup() {
  this->buf_.reserve(MAX_BUF);
  ESP_LOGCONFIG(TAG, "Pentair RS485 sniffer started (passive, receive-only)");
}

void PentairSniffer::loop() {
  // Drain everything the UART has buffered into our working buffer.
  uint8_t b;
  while (this->available()) {
    if (!this->read_byte(&b))
      break;
    this->buf_.push_back(b);
    if (this->buf_.size() > MAX_BUF)
      this->buf_.erase(this->buf_.begin());
  }

  // Parse greedily from the front of the buffer.  Each pass either consumes a
  // frame, drops a resync byte, or stops to wait for more data.
  bool progress = true;
  while (progress && !this->buf_.empty()) {
    progress = false;
    uint8_t b0 = this->buf_[0];
    if (b0 == 0xA5) {
      ParseResult r = this->parse_pentair_();
      if (r == PR_NEED_MORE)
        break;
      progress = true;  // OK and BAD both shortened the buffer
    } else if (b0 == 0x10 && this->buf_.size() >= 2 && this->buf_[1] == 0x02) {
      ParseResult r = this->parse_chlorinator_();
      if (r == PR_NEED_MORE)
        break;
      progress = true;
    } else {
      // Not a known start byte (preamble FF/00, or mid-frame noise) -> skip it.
      this->buf_.erase(this->buf_.begin());
      progress = true;
    }
  }

  this->publish_stats_();
}

PentairSniffer::ParseResult PentairSniffer::parse_pentair_() {
  // Layout from buf_[0]: A5 VER DST SRC CMD LEN DATA[LEN] CKH CKL
  if (this->buf_.size() < 6)
    return PR_NEED_MORE;

  uint8_t len = this->buf_[5];
  if (len > MAX_PENTAIR_LEN) {  // implausible length -> not a real frame
    this->buf_.erase(this->buf_.begin());
    return PR_BAD;
  }

  size_t total = 6 + (size_t) len + 2;
  if (this->buf_.size() < total)
    return PR_NEED_MORE;

  // Checksum = 16-bit sum of A5 through the last data byte, big-endian.
  uint16_t sum = 0;
  for (size_t i = 0; i < total - 2; i++)
    sum += this->buf_[i];
  uint16_t chk = (uint16_t(this->buf_[total - 2]) << 8) | this->buf_[total - 1];

  if (sum != chk) {
    this->cnt_cksum_err_++;
    this->stats_dirty_ = true;
    ESP_LOGV(TAG, "A5 frame bad checksum (calc=0x%04X got=0x%04X), resyncing", sum, chk);
    this->buf_.erase(this->buf_.begin());  // drop the A5 and try to resync
    return PR_BAD;
  }

  this->handle_pentair_(total);
  this->buf_.erase(this->buf_.begin(), this->buf_.begin() + total);
  return PR_OK;
}

void PentairSniffer::handle_pentair_(size_t total) {
  uint8_t dst = this->buf_[2];
  uint8_t src = this->buf_[3];
  uint8_t cmd = this->buf_[4];
  uint8_t len = this->buf_[5];

  this->cnt_valid_++;
  this->stats_dirty_ = true;

  std::vector<uint8_t> frame(this->buf_.begin(), this->buf_.begin() + total);
  std::string hex = format_hex_pretty(frame);

  bool is_pump = (dst == this->pump_address_) || (src == this->pump_address_);
  if (is_pump) {
    this->cnt_pump_++;
    // Status reply from the pump: cmd 0x07, src == pump, carries watts + rpm.
    //   data[3..4] = watts (BE), data[5..6] = rpm (BE), data[7] = gpm.
    if (cmd == 0x07 && src == this->pump_address_ && len >= 7) {
      uint16_t watts = (uint16_t(this->buf_[6 + 3]) << 8) | this->buf_[6 + 4];
      uint16_t rpm = (uint16_t(this->buf_[6 + 5]) << 8) | this->buf_[6 + 6];
      ESP_LOGI(TAG, "PUMP status  rpm=%u  watts=%u   %s", rpm, watts, hex.c_str());
      if (this->pump_rpm_sensor_ != nullptr)
        this->pump_rpm_sensor_->publish_state(rpm);
      if (this->pump_watts_sensor_ != nullptr)
        this->pump_watts_sensor_->publish_state(watts);
      if (len >= 8 && this->pump_gpm_sensor_ != nullptr)
        this->pump_gpm_sensor_->publish_state(this->buf_[6 + 7]);
    } else {
      ESP_LOGI(TAG, "PUMP frame   dst=0x%02X src=0x%02X cmd=0x%02X len=%u   %s", dst, src, cmd, len,
               hex.c_str());
    }
  } else if (this->log_all_) {
    ESP_LOGI(TAG, "A5 frame     dst=0x%02X src=0x%02X cmd=0x%02X len=%u   %s", dst, src, cmd, len,
             hex.c_str());
  }

  if (this->last_frame_sensor_ != nullptr)
    this->last_frame_sensor_->publish_state(hex);
}

PentairSniffer::ParseResult PentairSniffer::parse_chlorinator_() {
  // IntelliChlor: 10 02 DST CMD DATA... CHK 10 03.  We only structurally detect
  // and count these (the salt-cell decode lives in the IntelliChlor component);
  // seeing the counter rise confirms both devices share the bus.
  size_t end = 0;
  bool found = false;
  for (size_t i = 2; i + 1 < this->buf_.size() && i < MAX_CHLOR_FRAME; i++) {
    if (this->buf_[i] == 0x10 && this->buf_[i + 1] == 0x03) {
      end = i + 2;  // include the 10 03 terminator
      found = true;
      break;
    }
  }
  if (!found) {
    if (this->buf_.size() < MAX_CHLOR_FRAME)
      return PR_NEED_MORE;                  // terminator may still be arriving
    this->buf_.erase(this->buf_.begin());   // give up, resync past this 0x10
    return PR_BAD;
  }

  this->cnt_chlor_++;
  this->stats_dirty_ = true;
  if (this->log_all_) {
    std::vector<uint8_t> frame(this->buf_.begin(), this->buf_.begin() + end);
    ESP_LOGD(TAG, "IntelliChlor frame   %s", format_hex_pretty(frame).c_str());
  }
  this->buf_.erase(this->buf_.begin(), this->buf_.begin() + end);
  return PR_OK;
}

void PentairSniffer::publish_stats_() {
  if (!this->stats_dirty_)
    return;
  uint32_t now = millis();
  if (now - this->last_pub_ < 2000)  // rate-limit counter publishes
    return;
  this->last_pub_ = now;
  this->stats_dirty_ = false;
  if (this->valid_frames_sensor_ != nullptr)
    this->valid_frames_sensor_->publish_state(this->cnt_valid_);
  if (this->pump_frames_sensor_ != nullptr)
    this->pump_frames_sensor_->publish_state(this->cnt_pump_);
  if (this->chlorinator_frames_sensor_ != nullptr)
    this->chlorinator_frames_sensor_->publish_state(this->cnt_chlor_);
  if (this->checksum_errors_sensor_ != nullptr)
    this->checksum_errors_sensor_->publish_state(this->cnt_cksum_err_);
}

void PentairSniffer::dump_config() {
  ESP_LOGCONFIG(TAG, "Pentair RS485 Sniffer:");
  ESP_LOGCONFIG(TAG, "  Pump address: 0x%02X", this->pump_address_);
  ESP_LOGCONFIG(TAG, "  Log all frames: %s", YESNO(this->log_all_));
  this->check_uart_settings(9600);
}

}  // namespace pentair_sniffer
}  // namespace esphome
