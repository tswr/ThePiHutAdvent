#include "pico/stdlib.h"
#include <initializer_list>
#include <cstdio>

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

int main() {
  stdio_init_all();

  Led led0(25);
  Led led1(21);
  Led led2(20);
  Led led3(19);

  Button b1(2);
  Button b2(3);
  Button b3(4);

  while (1) {
    if (b1.is_pressed()) {
      led1.toggle();
    }
    if (b2.is_pressed()) {
      led2.toggle();
    }
    if (b3.is_pressed()) {
      led3.toggle();
    }

    sleep_ms(20);
  }

  return 0;
}
