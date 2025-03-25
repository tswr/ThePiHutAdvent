# ThePiHutAdvent

I've got this wonderful [The 12 Projects of
Codemas](https://thepihut.com/products/maker-advent-calendar-includes-raspberry-pi-pico-h)
advent callendar. Its original
[guides](https://thepihut.com/pages/maker-advent-2022-guides) are for
MicroPython, but I decided to take it to another level and do everything in C++
with [Pico SDK](https://github.com/raspberrypi/pico-sdk). 

**Spoiler alert**. Don't read further unless you want to know what each day
contained.

* [Day 1](https://github.com/tswr/ThePiHutAdvent/tree/main/day1) is blinking
with onboard LED.
* [Day 2](https://github.com/tswr/ThePiHutAdvent/tree/main/day2) featured four
LEDs, I wrote a small program to see how fast can they blink.
* [Day 3](https://github.com/tswr/ThePiHutAdvent/tree/main/day3) added three buttons.
* [Day 4](https://github.com/tswr/ThePiHutAdvent/tree/main/day4) added a knob. Here I decided to diverge from the main assignment and
made my own blinking metronome with the knob controlling the tempo and three
buttons switching between different modes.
* Day 5 added a buzzer. Here I went for two things:
  * [Day 5.1](https://github.com/tswr/ThePiHutAdvent/tree/main/day5.1) Buzzing the tune of Through fire and flames.
  * [Day 5.2](https://github.com/tswr/ThePiHutAdvent/tree/main/day5.2) Metronome with a buzzer.
* [Day 6](https://github.com/tswr/ThePiHutAdvent/tree/main/day6) had a light
sensor.
* [Day 7](https://github.com/tswr/ThePiHutAdvent/tree/main/day7) featured a
motion detection sensor.
* [Day 8](https://github.com/tswr/ThePiHutAdvent/tree/main/day8) had a
temperature sensor DS18B20. It was so much fun to learn about 1-wire protocol
and implement it myself.
* Day 9 I decided to skip. Tilt sensor did not like too much fun.
* Day 10 had a "break beam" which I also skipped.
* [Day 11](https://github.com/tswr/ThePiHutAdvent/blob/main/day11) had a OLED
display. Had lots of fun learning I2C and talking to the display through it.
First got the basic "show pixel" functionallity, then added outputing text and
reused the temperature sensor from Day 8 to report room temperature.
* [Day 12](https://github.com/tswr/ThePiHutAdvent/tree/main/day12) contained
WS2812 RGB LEDs. To get this one working I learned how to program PIO. Fantastic
feature.

Hope you to have fun with this set and find some helpful code over here to learn
more about Pico SDK.
