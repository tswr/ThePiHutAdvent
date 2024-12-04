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

private:
  const int pin_;
};

int main() {
  stdio_init_all();

  Led led0(25);
  Led led1(21);
  Led led2(20);
  Led led3(19);

  int factor = 1;
  while (1) {
    for (int i = 0; i < factor; ++i) {
      for (auto& led : {led0, led1, led2, led3}) {
        led.turnOnFor(1000 / factor);
      }
    }
    factor *= 2;
    printf("%d\n", factor);
    if (factor > 1024) {
      factor = 1;
    }
  }

  return 0;
}
