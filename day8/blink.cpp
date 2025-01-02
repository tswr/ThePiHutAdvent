#include <array>
#include <cerrno>
#include <cstddef>
#include <hardware/irq.h>
#include <limits>
#include <optional>

#include <cstdint>
#include <cstdio>

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

int main() {
  stdio_init_all();

  printf("Begin\n");
  sleep_ms(5000);
  DS18B20 sensor(26);
  while (1) {
    printf("Temperature: %f\n", *sensor.getTemperature());
    sleep_ms(1000);
  }
  return 0;
}
