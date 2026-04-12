#include <espos_core.h>

// -----------------------------------------------------------------------------
// This example demonstrates:
//
//   .init()       → initialization
//   .state()      → activate state
//   .on()         → direct control
//   .off()        → turn off
//   .setState()   → redefine a state’s output value at runtime
//   .getMode()    → LED type query
//
// The class uses the following functions:
//
//  mode 1: pwmLed(pin, invert, brightness)
//    uint8_t pin: gpio pin
//    bool    invert: false: LED to GND, true: LED to Vcc
//    uint8_t brightness: 0-31 (gamma corrected)
//
//  mode 2: neoLed(pin, color)
//   uint8_t  pin: gpio pin (WS2812 data)
//   String   color: RRGGBB hex (gamma corrected)
//
//  mode 3: dualLed(pin1, pin2, value)
//    uint8_t pin1: gpio pin (2pin bicolor led pin1)
//    uint8_t pin1: gpio pin (2pin bicolor led pin2)
//    uint8_t value:
//                0: off
//          1 -  31: color1 -> color1+color2 -> color2
//        100 - 131: color1 brightness (gamma corrected)
//        200 - 231: color2 brightness (gamma corrected) 
//
// !!!!! WARNING !!!!! DO NOT USE !!!!!
//   ESP8266: GPIO6, GPIO7, GPIO8, GPIO9, GPIO10, GPIO11
//   ESP32C3: GPIO12, GPIO13, GPIO14, GPIO15, GPIO16, GPIO17
//
// -----------------------------------------------------------------------------

Led led1;
Led led2;
Led led3;

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n\nStart LED Class demo\n\n");

  Serial.println("Initialize");

    led1.init(
        1,          // MODE 1 = LED  pwmLed(...) 
        2,          // Pin
        true,       // false: GND / true: Vcc
        "0",           // state 0 → off      
        "31",          // state 1 → on
        "6",           // state 2 → 20%
        "12",          // state 3 → 40%
        "18",          // state 4 → 60%
        "24"           // state 5 → 80%
    );

    led2.init(
        2,          // MODE 2 = Neopixel LED (WS2812 or compatible)  neoLed(...)
        14,         // Pin
        "000000",      // state 0 → OFF
        "FF0000",      // state 1 → red
        "00FF00",      // state 2 → green
        "0000FF",      // state 3 → blue
        "FFFF00",      // state 4 → yellow
        "FFFFFF"       // state 5 → white
    );

    led3.init(
        3,         // MODE 3 = Dual LED (2 pin bicolor)  dualLed(...)  
        4,         // Pin1
        5,         // Pin2
        "0",          // state 0 → OFF
        "131",        // state 1 → color1 on
        "231",        // state 2 → color2 on
        "15",         // state 3 → color1 + color2 (50% / 50%)
        "115",        // state 4 → color1 50%
        "215"         // state 5 → color2 50%
    );

  Serial.println("On state");   
    stateDemo(); // .state(x)
    delay(2000);

  Serial.println("Off");
    led1.off();
    led2.off();
    led3.off();
    delay(2000);
    
  Serial.println("On fix value");
    led1.on(31);
    led2.on("FFFFFF");
    led3.on(15);
    delay(2000);

  Serial.println("Set state");
    led1.setState(0, "2");
    led2.setState(0, "002200");
    led3.setState(0, "102");

  Serial.print("Get mode: ");
    if (led1.getMode() == 0) Serial.println("Not initialized");
    if (led1.getMode() == 1) Serial.println("Normal LED");
    if (led1.getMode() == 2) Serial.println("Neopixel LED");
    if (led1.getMode() == 3) Serial.println("Bicolor LED");

  Serial.println("loop state0 - stste5");
 
}

void stateDemo() {
    for (uint8_t i = 0; i <= 5; i++) {
        led1.state(i);   // 
        led2.state(i);   // 
        led3.state(i);   // 
        delay(800);
    }
}
    
void loop() {
  stateDemo();
}
