#include <cstdio>
#include <initializer_list>

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

class Led {
public:
  explicit Led(int pin): pin_(pin) {
    gpio_init(pin_);
    gpio_set_dir(pin_, GPIO_OUT);
  }

  void turnOnFor(int ms) const {
    gpio_put(pin_, true);
    sleep_ms(ms);
    gpio_put(pin_, false);
  }

  void turnOn() const {
    gpio_put(pin_, true);
  }

  void turnOff() const {
    gpio_put(pin_, false);
  }

  void toggle() const {
    gpio_put(pin_, !gpio_get_out_level(pin_));
  }

private:
  const int pin_;
};

class Button {
public:
  explicit Button(int pin): pin_(pin) {
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
  explicit Knob(int pin, int adc_input): pin_(pin) {
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

enum class Subdivision {
  QUARTERS = 1,
  EIGHTHS,
  TRIPPLETS
};

int main() {
  constexpr float maxBpm = 250;
  constexpr float minBpm = 40;

  stdio_init_all();

  Led led0(25);
  Led led1(21);
  Led led2(20);
  Led led3(19);

  Button b1(2);
  Button b2(3);
  Button b3(4);

  Knob knob(26, 0);

  auto mode = Subdivision::QUARTERS;

  while (1) {
    if (b1.is_pressed()) {
      mode = Subdivision::QUARTERS;
    }
    if (b2.is_pressed()) {
      mode = Subdivision::EIGHTHS;
    }
    if (b3.is_pressed()) {
      mode = Subdivision::TRIPPLETS;
    }

    const float bpm = (maxBpm - minBpm) * knob.read() + minBpm;

    int repeats = static_cast<int>(mode);
    printf("mode = %d\n", repeats);
    printf("bpm = %f\n", bpm);

    const float duration = 1000 * (60 / (bpm * repeats)); // in ms

    for (const auto& led : {led0, led1, led2, led3}) {
      for (int r = 0; r < repeats; ++r) {
        led.turnOnFor(duration / 2);
        sleep_ms(duration / 2);
      }
    }
  }

  return 0;
}
