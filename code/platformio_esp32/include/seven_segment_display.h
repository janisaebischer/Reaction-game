#ifndef SEVEN_SEGMENT_DISPLAY_H
#define SEVEN_SEGMENT_DISPLAY_H

#include <Arduino.h>
#include <array>

#include "settings.h"

class SevenSegmentDisplay {
 public:
  bool begin() {
    // Driver 1
    pinMode(SEVENSEG_DATA_PIN_1, OUTPUT);
    pinMode(SEVENSEG_CLOCK_PIN_1, OUTPUT);
    pinMode(SEVENSEG_LATCH_PIN_1, OUTPUT);
    digitalWrite(SEVENSEG_DATA_PIN_1, LOW);
    digitalWrite(SEVENSEG_CLOCK_PIN_1, LOW);
    digitalWrite(SEVENSEG_LATCH_PIN_1, LOW);
    // Driver 2
    pinMode(SEVENSEG_DATA_PIN_2, OUTPUT);
    pinMode(SEVENSEG_CLOCK_PIN_2, OUTPUT);
    pinMode(SEVENSEG_LATCH_PIN_2, OUTPUT);
    digitalWrite(SEVENSEG_DATA_PIN_2, LOW);
    digitalWrite(SEVENSEG_CLOCK_PIN_2, LOW);
    digitalWrite(SEVENSEG_LATCH_PIN_2, LOW);
    // Driver 3
    pinMode(SEVENSEG_DATA_PIN_3, OUTPUT);
    pinMode(SEVENSEG_CLOCK_PIN_3, OUTPUT);
    pinMode(SEVENSEG_LATCH_PIN_3, OUTPUT);
    digitalWrite(SEVENSEG_DATA_PIN_3, LOW);
    digitalWrite(SEVENSEG_CLOCK_PIN_3, LOW);
    digitalWrite(SEVENSEG_LATCH_PIN_3, LOW);
    // Driver 4
    pinMode(SEVENSEG_DATA_PIN_4, OUTPUT);
    pinMode(SEVENSEG_CLOCK_PIN_4, OUTPUT);
    pinMode(SEVENSEG_LATCH_PIN_4, OUTPUT);
    digitalWrite(SEVENSEG_DATA_PIN_4, LOW);
    digitalWrite(SEVENSEG_CLOCK_PIN_4, LOW);
    digitalWrite(SEVENSEG_LATCH_PIN_4, LOW);

    clear();
    ready_ = true;
    return true;
  }

  bool isReady() const {
    return ready_;
  }

  String text() const {
    return text_;
  }

  bool showText(const String& input) {
    std::array<uint8_t, 4> raw{};
    raw.fill(encodeGlyph(' '));

    String normalized;
    normalized.reserve(5);

    int digitIndex = 0;
    for (size_t i = 0; i < input.length() && digitIndex < 4; ++i) {
      const char c = input[i];
      if (c == '.') {
        if (digitIndex > 0) {
          raw[digitIndex - 1] |= kDecimalPointMask;
        }
        normalized += c;
        continue;
      }

      raw[digitIndex] = encodeGlyph(c);
      normalized += c;
      ++digitIndex;
    }

    while (digitIndex < SEVENSEG_DIGIT_COUNT) {
      raw[digitIndex++] = encodeGlyph(' ');
    }

    if (!writeRaw(raw)) {
      return false;
    }

    text_ = normalized;
    return true;
  }

  bool showValue(long value, int decimalPoint = -1) {
    String text = String(value);
    if (decimalPoint >= 0 && decimalPoint < static_cast<int>(text.length())) {
      text = text.substring(0, decimalPoint) + "." + text.substring(decimalPoint);
    }
    return showText(text);
  }

  bool showTime(unsigned long milliseconds) {
    unsigned int seconds = (milliseconds / 1000) % 100;
    unsigned int hundredths = (milliseconds / 10) % 100;

    char buffer[6];
    snprintf(buffer, sizeof(buffer), "%02u.%02u", seconds, hundredths);

    return showText(buffer);
  }

  bool clear() {
    std::array<uint8_t, SEVENSEG_DIGIT_COUNT> raw{};
    raw.fill(encodeGlyph(' '));
    text_.remove(0);
    return writeRaw(raw);
  }

 private:
  static constexpr uint8_t kSegmentAMask = 0x01; // bit0
  static constexpr uint8_t kSegmentFMask = 0x02; // bit1
  static constexpr uint8_t kSegmentGMask = 0x04; // bit2
  static constexpr uint8_t kSegmentEMask = 0x08; // bit3
  static constexpr uint8_t kSegmentDMask = 0x10; // bit4
  static constexpr uint8_t kSegmentCMask = 0x20; // bit5
  static constexpr uint8_t kSegmentBMask = 0x40; // bit6
  static constexpr uint8_t kDecimalPointMask = 0x80; // bit7

  bool ready_ = false;
  String text_;

  uint8_t encodeGlyph(char c) const {
    uint8_t value = 0;

    switch (c) {
      case '0': value = kSegmentAMask | kSegmentBMask | kSegmentCMask | kSegmentDMask | kSegmentEMask | kSegmentFMask; break;
      case '1': value = kSegmentBMask | kSegmentCMask; break;
      case '2': value = kSegmentAMask | kSegmentBMask | kSegmentDMask | kSegmentEMask | kSegmentGMask; break;
      case '3': value = kSegmentAMask | kSegmentBMask | kSegmentCMask | kSegmentDMask | kSegmentGMask; break;
      case '4': value = kSegmentBMask | kSegmentCMask | kSegmentFMask | kSegmentGMask; break;
      case '5': value = kSegmentAMask | kSegmentCMask | kSegmentDMask | kSegmentFMask | kSegmentGMask; break;
      case '6': value = kSegmentAMask | kSegmentCMask | kSegmentDMask | kSegmentEMask | kSegmentFMask | kSegmentGMask; break;
      case '7': value = kSegmentAMask | kSegmentBMask | kSegmentCMask; break;
      case '8': value = kSegmentAMask | kSegmentBMask | kSegmentCMask | kSegmentDMask | kSegmentEMask | kSegmentFMask | kSegmentGMask; break;
      case '9': value = kSegmentAMask | kSegmentBMask | kSegmentCMask | kSegmentDMask | kSegmentFMask | kSegmentGMask; break;
      case 'A': case 'a': value = kSegmentAMask | kSegmentBMask | kSegmentCMask | kSegmentEMask | kSegmentFMask | kSegmentGMask; break;
      case 'B': case 'b': value = kSegmentCMask | kSegmentDMask | kSegmentEMask | kSegmentFMask | kSegmentGMask; break;
      case 'C': case 'c': value = kSegmentAMask | kSegmentDMask | kSegmentEMask | kSegmentFMask; break;
      case 'D': case 'd': value = kSegmentBMask | kSegmentCMask | kSegmentDMask | kSegmentEMask | kSegmentGMask; break;
      case 'E': case 'e': value = kSegmentAMask | kSegmentDMask | kSegmentEMask | kSegmentFMask | kSegmentGMask; break;
      case 'F': case 'f': value = kSegmentAMask | kSegmentEMask | kSegmentFMask | kSegmentGMask; break;
      case '-': value = kSegmentGMask; break;
      case '_': value = kSegmentDMask; break;
      case ' ': default: value = 0; break;
    }

    if (SEVENSEG_ACTIVE_LOW) {
      value = ~value;
    }

    return value;
  }

  void shiftOutWithBitDelay(uint8_t dataPin,
                            uint8_t clockPin,
                            uint8_t bitOrder,
                            uint8_t value,
                            uint16_t bitDelayUs)
  {
    for (uint8_t i = 0; i < 8; i++) {

      uint8_t bitIndex = (bitOrder == MSBFIRST) ? (7 - i) : i;
      bool bit = (value >> bitIndex) & 0x01;

      digitalWrite(clockPin, LOW);
      digitalWrite(dataPin, bit);
      delayMicroseconds(bitDelayUs / 2);

      digitalWrite(clockPin, HIGH);
      delayMicroseconds(bitDelayUs / 2);
    }

    digitalWrite(clockPin, LOW);
  }

  bool writeRaw(const std::array<uint8_t, SEVENSEG_DIGIT_COUNT>& raw) {

    digitalWrite(SEVENSEG_LATCH_PIN_1, LOW);
    digitalWrite(SEVENSEG_LATCH_PIN_2, LOW);
    digitalWrite(SEVENSEG_LATCH_PIN_3, LOW);
    digitalWrite(SEVENSEG_LATCH_PIN_4, LOW);

    shiftOutWithBitDelay(SEVENSEG_DATA_PIN_1, SEVENSEG_CLOCK_PIN_1, MSBFIRST, raw[0], 10);
    shiftOutWithBitDelay(SEVENSEG_DATA_PIN_2, SEVENSEG_CLOCK_PIN_2, MSBFIRST, raw[1], 10);
    shiftOutWithBitDelay(SEVENSEG_DATA_PIN_3, SEVENSEG_CLOCK_PIN_3, MSBFIRST, raw[2], 10);
    shiftOutWithBitDelay(SEVENSEG_DATA_PIN_4, SEVENSEG_CLOCK_PIN_4, MSBFIRST, raw[3], 10);

    digitalWrite(SEVENSEG_LATCH_PIN_1, HIGH);
    digitalWrite(SEVENSEG_LATCH_PIN_2, HIGH);
    digitalWrite(SEVENSEG_LATCH_PIN_3, HIGH);
    digitalWrite(SEVENSEG_LATCH_PIN_4, HIGH);

    return true;
  }
};

extern SevenSegmentDisplay sevenSegDisplay;

#endif // SEVEN_SEGMENT_DISPLAY_H