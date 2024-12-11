#include <array>

#include <cstdint>
#include <cstdio>

#include <hardware/structs/io_bank0.h>
#include <initializer_list>
#include <pico.h>
#include <pico/time.h>

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

class Knob {
public:
  explicit Knob(int pin, int adc_input) : pin_(pin) {
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

private:
  const int pin_;
  const uint sliceNum_;
  const uint channel_;
  const uint32_t sysClockHz_;
  const float clockDivider_;
};

int main() {
  // constexpr float maxBpm = 250;
  // constexpr float minBpm = 40;
  //
  // stdio_init_all();
  //
  // Led led0(25);
  // Led led1(21);
  // Led led2(20);
  // Led led3(19);
  //
  // Button b1(2);
  // Button b2(3);
  // Button b3(4);
  //
  // Knob knob(26, 0);
  //
  // auto mode = Subdivision::QUARTERS;
  //
  // while (1) {
  //   if (b1.is_pressed()) {
  //     mode = Subdivision::QUARTERS;
  //   }
  //   if (b2.is_pressed()) {
  //     mode = Subdivision::EIGHTHS;
  //   }
  //   if (b3.is_pressed()) {
  //     mode = Subdivision::TRIPPLETS;
  //   }
  //
  //   const float bpm = (maxBpm - minBpm) * knob.read() + minBpm;
  //
  //   int repeats = static_cast<int>(mode);
  //   printf("mode = %d\n", repeats);
  //   printf("bpm = %f\n", bpm);
  //
  //   const float duration = 1000 * (60 / (bpm * repeats)); // in ms
  //
  //   for (const auto& led : {led0, led1, led2, led3}) {
  //     for (int r = 0; r < repeats; ++r) {
  //       led.turnOnFor(duration / 2);
  //       sleep_ms(duration / 2);
  //     }
  //   }
  // }

  stdio_init_all();

  std::array<float, 25> notes = {261.63f, 277.18f, 293.66f, 311.13f, 329.63f,
                                 349.23f, 369.99f, 392.00f, 415.30f, 440.00f,
                                 466.16f, 493.88f, 523.25f, 554.37f, 587.33f,
                                 622.25f, 659.25f, 698.46f, 739.99f, 783.99f,
                                 830.61f, 880.00f, 932.33f, 987.77f, 1046.50f};

  std::array<int, 15> minorScale = {0,  2,  3,  5,  7,  9,  10, 12,
                                    14, 15, 17, 19, 21, 22, 24};

  Buzzer buzzer(13);

  std::array<int, 16 * 7 + 8> throughFireAndFlamesP1 = {
      7, 8, 9, 7, 8, 9, 10, 8, 11, 9, 10, 8, 9, 7, 8, 6, //
      7, 8, 9, 7, 8, 9, 10, 8, 11, 9, 10, 8, 9, 7, 8, 6, //
      7, 8, 9, 7, 8, 9, 10, 8, 11, 9, 10, 8, 9, 7, 8, 6, //
      7, 8, 9, 7, 8, 9, 10, 8, 11, 9, 10, 8, 9, 7, 8, 6, //
      7, 8, 9, 7, 8, 9, 10, 8, 11, 9, 10, 8, 9, 7, 8, 6, //
      7, 8, 9, 7, 8, 9, 10, 8, 11, 9, 10, 8, 9, 7, 8, 6, //
      7, 8, 9, 7, 8, 9, 10, 8, 11, 9, 10, 8, 9, 7, 8, 6, //
      7, 8, 9, 7, 8, 9, 10, 8,
  };

  std::array<int, 16 * 5> throughFireAndFlamesP2 = {
      13, 10, 9, 7, 6,  7, 9,  10, 12, 11, 13, 12, 11, 10, 9,  10, //
      11, 7,  7, 7, 9,  7, 11, 7,  7,  7,  9,  7,  11, 7,  9,  7,  //
      11, 7,  7, 7, 9,  7, 11, 7,  7,  7,  9,  7,  11, 7,  9,  7,  //
      9,  5,  5, 5, 12, 5, 9,  5,  5,  5,  12, 5,  9,  5,  12, 5,  //
      9,  5,  5, 5, 12, 5, 9,  5,  5,  5,  12, 5,  9,  5,  12, 5,  //
  };

  while (1) {
    for (int i = 0; i < throughFireAndFlamesP1.size(); ++i) {
      buzzer.playFrequencyFor(notes[minorScale[throughFireAndFlamesP1[i]]],
                              150.f);
    }
    for (int i = 0; i < throughFireAndFlamesP2.size(); ++i) {
      buzzer.playFrequencyFor(notes[minorScale[throughFireAndFlamesP2[i]]],
                              75.f);
    }
  }

  return 0;
}
