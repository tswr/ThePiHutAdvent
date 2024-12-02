#include "pico/stdlib.h"

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

private:
  const int pin_;
};

int main() {
  Led led(25);

  while (true) {
    led.turnOnFor(1000);
    sleep_ms(250);
  }

  return 0;
}
