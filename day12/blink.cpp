#include <array>
#include <cerrno>
#include <cstddef>
#include <hardware/irq.h>
#include <limits>
#include <optional>
#include <string>

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include <hardware/i2c.h>
#include <hardware/structs/io_bank0.h>
#include <initializer_list>
#include <pico.h>
#include <pico/time.h>
#include <type_traits>

#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"

#include "ws2812.pio.h"

class Led {
public:
  explicit Led(int pin) : pin_(pin) {
    gpio_init(pin_);
    gpio_set_dir(pin_, GPIO_OUT);
  }

  void turnOnFor(int ms) const {
    gpio_put(pin_, true);
    sleep_ms(ms);
    gpio_put(pin_, false);
  }

  void turnOn() const { gpio_put(pin_, true); }

  void turnOff() const { gpio_put(pin_, false); }

  void toggle() const { gpio_put(pin_, !gpio_get_out_level(pin_)); }

private:
  const int pin_;
};

class Button {
public:
  explicit Button(int pin) : pin_(pin) {
    gpio_init(pin_);
    gpio_set_dir(pin_, GPIO_IN);
    gpio_pull_down(pin_);
  }

  bool is_pressed() {
    bool state = gpio_get(pin_);
    if (state) {
      if (!is_pressed_) {
        is_pressed_ = true;
        return true;
      }
    } else {
      is_pressed_ = false;
    }
    return false;
  }

private:
  int pin_;
  bool is_pressed_ = {};
};

class PassiveInfraRedSensor {
public:
  explicit PassiveInfraRedSensor(int pin) : pin_(pin) {
    gpio_init(pin_);
    gpio_set_dir(pin_, GPIO_IN);
    gpio_pull_down(pin_);
    printf("Starting PIR warm up...\n");
    sleep_ms(10'000); // Warm up
    printf("PIR warm up finished\n");
  }

  bool hasDetection() const {
    const bool state = gpio_get(pin_);
    return state;
  }

private:
  int pin_;
};

class AdcReader {
public:
  explicit AdcReader(int pin, int adc_input) : pin_(pin) {
    adc_init();
    adc_gpio_init(pin_);
    adc_select_input(adc_input);
  }

  float read() const {
    uint16_t result = adc_read();
    return float(result) / 4096;
  }

private:
  int pin_;
};

enum class Subdivision { QUARTERS = 1, EIGHTHS, TRIPPLETS };

class Buzzer {
public:
  explicit Buzzer(int pin)
      : pin_(pin), sliceNum_(pwm_gpio_to_slice_num(pin_)),
        channel_(pwm_gpio_to_channel(pin_)), sysClockHz_(clock_get_hz(clk_sys)),
        clockDivider_(125.f) {
    gpio_set_function(pin_, GPIO_FUNC_PWM);
    pwm_set_clkdiv(sliceNum_, clockDivider_);
    pwm_set_enabled(sliceNum_, true);
  }

  ~Buzzer() { pwm_set_enabled(sliceNum_, false); }

private:
  uint16_t convertFrequencyToWrap(const float targetFrequencyHz) {
    const float noteDuration = 1.f / targetFrequencyHz;
    const float tickDuration =
        1.f / (static_cast<float>(sysClockHz_) / clockDivider_);
    const uint16_t ticksPerNote =
        static_cast<uint16_t>(noteDuration / tickDuration);
    return ticksPerNote - 1;
  }

public:
  void playFrequencyFor(const float frequency, const float durationMs) {
    const uint16_t wrap = convertFrequencyToWrap(frequency);
    pwm_set_wrap(sliceNum_, wrap);
    pwm_set_chan_level(sliceNum_, channel_, wrap / 4);
    sleep_ms(durationMs);
  }

  void off() { pwm_set_chan_level(sliceNum_, channel_, 0); }

  void playFrequencyFor(const float frequency1, const float frequency2,
                        const float durationMs, uint64_t oneNoteDurationUs) {
    const uint16_t wrap1 = convertFrequencyToWrap(frequency1);
    const uint16_t wrap2 = convertFrequencyToWrap(frequency2);
    uint64_t iterations = 1000 * durationMs / oneNoteDurationUs / 2;
    for (uint64_t i = 0; i < iterations; ++i) {
      pwm_set_wrap(sliceNum_, wrap1);
      pwm_set_chan_level(sliceNum_, channel_, wrap1 / 16);
      sleep_us(oneNoteDurationUs);
      pwm_set_wrap(sliceNum_, wrap2);
      pwm_set_chan_level(sliceNum_, channel_, wrap2 / 16);
      sleep_us(oneNoteDurationUs);
    }
  }

private:
  const int pin_;
  const uint sliceNum_;
  const uint channel_;
  const uint32_t sysClockHz_;
  const float clockDivider_;
};

class DS18B20 {
public:
  explicit DS18B20(int pin) : pin_(pin) {}

  std::optional<float> getTemperature() {
    if (!initialize()) {
      return {};
    }
    // printf("rom: %llu\n", readRom());
    skipRom();

    // Convert T
    constexpr uint8_t CONVERT_T = 0x44;
    printf("Converting temperature started\n");
    writeByte(CONVERT_T);
    while (!readBit()) {
    }
    printf("Converting temperature finished\n");

    if (!initialize()) {
      return {};
    }
    // printf("rom: %llu\n", readRom());
    skipRom();
    // Read scratchpad
    printf("Reading temperature\n");
    constexpr uint8_t READ_SCRATCHPAD = 0xBE;
    writeByte(READ_SCRATCHPAD);
    std::array<uint8_t, 9> scratchpad{0};
    readBytes(scratchpad);

    printf("scratchpad:\n");
    for (const auto &byte : scratchpad) {
      printf("%d\n", byte);
    }
    return decodeTemperature(scratchpad[0], scratchpad[1]);
  }

private:
  bool initialize() {
    printf("Initializing\n");
    gpio_init(pin_);

    // Reset pulse
    printf("Reset pulse\n");
    gpio_set_dir(pin_, GPIO_OUT);
    gpio_put(pin_, 0);
    sleep_us(480);

    // Check for response
    gpio_set_dir(pin_, GPIO_IN);
    sleep_us(60);
    const bool isPresent = (gpio_get(pin_) == 0);
    sleep_us(240);
    const bool wasReleased = (gpio_get(pin_) == 1);
    sleep_us(240);
    printf("isPresent = %d, wasReleased = %d\n", isPresent, wasReleased);
    return isPresent && wasReleased;
  }

  void writeBit(bool bit) {
    gpio_set_dir(pin_, GPIO_OUT);
    if (bit) {
      gpio_put(pin_, 0);
      sleep_us(5);
      gpio_set_dir(pin_, GPIO_IN);
      sleep_us(55);
    } else {
      gpio_put(pin_, 0);
      sleep_us(60);
      gpio_set_dir(pin_, GPIO_IN);
      sleep_us(10);
    }
  }

  bool readBit() {
    gpio_set_dir(pin_, GPIO_OUT);
    gpio_put(pin_, 0);
    sleep_us(5);

    gpio_set_dir(pin_, GPIO_IN);
    sleep_us(10);
    const bool bit = gpio_get(pin_);
    sleep_us(55);

    return bit;
  }

  void writeByte(uint8_t byte) {
    printf("Writing byte: %02X\n", byte);
    for (int i = 0; i < 8; ++i) {
      writeBit(byte & 1);
      byte >>= 1;
    }
  }

  uint8_t readByte() {
    uint8_t byte = 0;
    for (int i = 0; i < 8; ++i) {
      if (readBit()) {
        byte |= (1 << i);
      }
    }
    printf("Read byte: %02X\n", byte);
    return byte;
  }

  template <size_t N> void readBytes(std::array<uint8_t, N> &bytes) {
    for (auto &byte : bytes) {
      byte = readByte();
    }
  }

  uint64_t readRom() {
    writeByte(0x33);
    uint64_t rom;
    for (int i = 0; i < 4; ++i) {
      rom |= (readByte() << (i * 8));
    }
    return rom;
    // My rom was 4294967295
  }

  void skipRom() { writeByte(0xCC); }

  float decodeTemperature(uint8_t lsb, uint8_t msb) {
    int16_t rawTemperature =
        static_cast<int16_t>(lsb) | (static_cast<int16_t>(msb) << 8);
    float temperature = rawTemperature / 16.0f;
    return temperature;
  }

private:
  int pin_;
};

constexpr std::array<std::array<uint8_t, 5>, 96> kFont5x8 = {{
    {0x00, 0x00, 0x00, 0x00, 0x00}, // ' ' (space)
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // '"'
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // '#'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '''
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // '('
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // ')'
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // '*'
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // '/'
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ';'
    {0x08, 0x14, 0x22, 0x41, 0x00}, // '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // '='
    {0x00, 0x41, 0x22, 0x14, 0x08}, // '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // '?'
    {0x3E, 0x41, 0x5D, 0x59, 0x4E}, // '@'
    {0x7C, 0x12, 0x11, 0x12, 0x7C}, // 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 'C'
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 'D'
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 'E'
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 'F'
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 'G'
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 'H'
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 'I'
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 'J'
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 'K'
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 'L'
    {0x7F, 0x02, 0x1C, 0x02, 0x7F}, // 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 'V'
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 'Z'
}};

class Framebuffer {
public:
  Framebuffer() { clear(); }

  const uint8_t *data() const { return buffer_.data(); }

  void clear() { std::fill(buffer_.begin(), buffer_.end(), 0x00); }

  void setPixel(int x, int y) {
    printf("Setting pixel at x = %d, y = %d\n", x, y);
    const auto [index, bit] = toIndex(x, y);
    buffer_[index] |= 1 << bit;
  }

  void unsetPixel(int x, int y) {
    const auto [index, bit] = toIndex(x, y);
    buffer_[index] &= ~(1 << bit);
  }

  void putLetter(int x, int y, char c) {
    const auto &glyph = kFont5x8[c - 32];
    for (int w = 0; w < glyph.size(); ++w) {
      for (int h = 0; h < 8; ++h) {
        int bit = (glyph[w] >> h) & 1;
        if (bit) {
          setPixel(x + w, y + h);
        }
      }
    }
  }

  void putText(int x, int y, const std::string &text) {
    for (int i = 0; i < text.size(); ++i) {
      putLetter(x + i * 7, y, std::toupper(text[i]));
    };
  }

private:
  std::pair<int, int> toIndex(int x, int y) {
    int page = y / kPageHeight;
    int index = x + (page * kWidth);
    int bit = y % 8;
    return {index, bit};
  }

private:
  static constexpr int kWidth = 128;
  static constexpr int kHeight = 32;
  static constexpr int kPages = 4;
  static constexpr int kPageHeight = kHeight / kPages;
  std::array<uint8_t, kWidth * kPages> buffer_;
};

class SSD1906 {
public:
  SSD1906(int sdaPin, int sclPin) {
    i2c_init(i2c0, 400000);
    gpio_set_function(sdaPin, GPIO_FUNC_I2C);
    gpio_set_function(sclPin, GPIO_FUNC_I2C);
    gpio_pull_up(sdaPin);
    gpio_pull_up(sclPin);
    uint8_t init_sequence[] = {
        0x00,       // Control byte: command
        0xAE,       // Display OFF
        0xD5, 0x80, // Set display clock divide ratio/oscillator frequency
        0xA8, 0x1F, // Set multiplex ratio (31 for 128x32)
        0xD3, 0x00, // Set display offset to 0
        0x40,       // Set start line to 0
        0x8D, 0x14, // Enable charge pump
        0x20, 0x00, // Set memory addressing mode to horizontal
        0xA1,       // Set segment re-map (horizontal flip)
        0xC8,       // Set COM output scan direction (vertical flip)
        0xDA, 0x02, // Set COM pins hardware configuration
        0x81, 0x7F, // Set contrast (128)
        0xD9, 0xF1, // Set pre-charge period
        0xDB, 0x40, // Set VCOMH deselect level
        0xA4,       // Entire display ON (resume RAM content display)
        0xA6,       // Normal display (not inverted)
        0xAF        // Display ON
    };
    i2c_write_blocking(i2c0, 0x3C, init_sequence, sizeof(init_sequence), false);
  }

  void show(const Framebuffer &framebuffer) {
    std::array<uint8_t, 129> buffer;
    buffer[0] = 0x40;
    for (uint8_t page = 0; page < 4; ++page) {
      std::array<uint8_t, 4> pageAddress = {0x00, 0xB0 + page, 0x00, 0x10};
      i2c_write_blocking(i2c0, 0x3C, pageAddress.data(), pageAddress.size(),
                         false);
      memcpy(buffer.data() + 1, &framebuffer.data()[page * 128], 128);
      i2c_write_blocking(i2c0, 0x3C, buffer.data(), buffer.size(), false);
    }
  }
};

template <size_t N> class WS2812 {
public:
  explicit WS2812(int pin) : pin_(pin) {
    gpio_init(pin_);
    gpio_set_dir(pin_, GPIO_OUT);
  }
  struct Color {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
  };

  void setColors(const std::array<Color, N> &colors) {
    for (const auto &color : colors) {
      sendColor(color);
    }
    resetLine();
  }

private:
  static constexpr int kTime0High = 400;
  static constexpr int kTime0Low = 850;
  static constexpr int kTime1High = 800;
  static constexpr int kTime1Low = 450;
  static constexpr int kResetTime = 50000;

  static constexpr int kCycles0High = 53;
  static constexpr int kCycles0Low = 106;
  static constexpr int kCycles1High = 106;
  static constexpr int kCycles1Low = 53;
  static constexpr int kResetCycles = 8000;

  void delay_cycles(uint32_t cycles) {
    while (cycles--) {
      __asm__ __volatile__("nop");
    }
  }

  void sendBit(bool bit) {
    // if (bit) {
    //   gpio_put(pin_, 1);
    //   sleep_us(kTime1High);
    //   gpio_put(pin_, 0);
    //   sleep_us(kTime1Low);
    // } else {
    //   gpio_put(pin_, 1);
    //   sleep_us(kTime0High);
    //   gpio_put(pin_, 0);
    //   sleep_us(kTime0Low);
    // }
    if (bit) {
      sio_hw->gpio_set = (1u << pin_); // High
      delay_cycles(kCycles1High);      // T1H
      sio_hw->gpio_clr = (1u << pin_); // Low
      delay_cycles(kCycles1Low);       // T1L
    } else {
      sio_hw->gpio_set = (1u << pin_); // High
      delay_cycles(kCycles0High);      // T0H
      sio_hw->gpio_clr = (1u << pin_); // Low
      delay_cycles(kCycles0Low);       // T0L
    }
  }

  void sendByte(uint8_t byte) {
    // Most significant bit first
    for (int i = 7; i >= 0; i--) {
      sendBit((byte >> i) & 0x01);
    }
  }

  void sendColor(const Color &color) {
    // Sent in GRB order
    sendByte(color.green);
    sendByte(color.red);
    sendByte(color.blue);
  }

  void resetLine() {
    sio_hw->gpio_clr = (1u << pin_);
    delay_cycles(kResetCycles);
  }

private:
  int pin_;
};

int mainNN() {
  stdio_init_all();

  // DS18B20 sensor(26);
  // SSD1906 oled(16, 17);
  // sleep_ms(5000);
  //
  // Framebuffer framebuffer;
  //
  // while (1) {
  //   framebuffer.clear();
  //   std::string output =
  //       "Temp: " + std::to_string(*sensor.getTemperature()) + " C";
  //   framebuffer.putText(0, 12, output);
  //   oled.show(framebuffer);
  //   sleep_ms(1000);
  // }

  std::array<WS2812<15>::Color, 15> colors = {{{255, 0, 0},
                                               {0, 255, 0},
                                               {0, 0, 255},
                                               {255, 0, 0},
                                               {0, 255, 0},
                                               {0, 0, 255},
                                               {255, 0, 0},
                                               {0, 255, 0},
                                               {0, 0, 255},
                                               {255, 0, 0},
                                               {0, 255, 0},
                                               {0, 0, 255},
                                               {255, 0, 0},
                                               {0, 255, 0},
                                               {0, 0, 255}}};

  WS2812<15> ledStrip(28);
  while (1) {
    ledStrip.setColors(colors);
    sleep_ms(1000);
  }

  return 0;
}

static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
  pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

void pattern_random(PIO pio, uint sm, uint len) {
  for (uint i = 0; i < len; ++i)
    put_pixel(pio, sm, rand());
}

int main() {
  stdio_init_all();

  // todo get free sm
  PIO pio;
  uint sm;
  uint offset;

  constexpr uint kWs2812Pin = 28;
  constexpr bool isRGBW = false;
  constexpr int numPixels = 15;

  bool success = pio_claim_free_sm_and_add_program_for_gpio_range(
      &ws2812_program, &pio, &sm, &offset, kWs2812Pin, 1, true);
  hard_assert(success);

  ws2812_program_init(pio, sm, offset, kWs2812Pin, 800000, isRGBW);
  while (1) {
    std::array<uint32_t, numPixels> state = {0};
    for (int k = numPixels; k > 0; --k) {
      const auto pixel = rand();

      for (int i = 0; i < k; ++i) {
        if (i != 0) {
          state[i - 1] = 0;
        }
        state[i] = pixel;
        for (const auto p : state) {
          put_pixel(pio, sm, p);
        }
        sleep_ms(100);
      }
    }
    sleep_ms(2000);
  }

  pio_remove_program_and_unclaim_sm(&ws2812_program, pio, sm, offset);
}
