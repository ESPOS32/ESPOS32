#include <espos_core.h>

// -----------------------------------------------------------------------------
// The BlinkTask class is a lightweight, non-blocking state machine for timed LED sequences.
// It provides deterministic phase transitions and precise timing without using delays.
// Each instance runs independently, while a static scheduler updates all tasks globally.
// Callbacks use std::function, allowing capture-enabled lambdas and bound methods.
// Ideal for system-level indicators such as WiFi/MQTT status, error states, or animations.
//
// This examples demonstrates:
//
//   .start()              → Starts the sequence and enters the first phase.
//   .stop()               → Immediately finishes the sequence and runs the finish callback.
//   .end()                → Forces the current cycle to end and transitions to finished state.
//   .pause()              → Suspends the sequence, preserving the current phase timing.
//   .resume()             → Continues the sequence from the exact paused position.
//   .phase()              → Gets the current internal phase/state.
//   .elapsedPhase()       → Returns elapsed time in the current phase. (Counter runs during PAUSE)
//   .remainingPhase()     → Returns remaining time before the current phase ends. (Counter runs during PAUSE)
//   .elapsedTotal()       → Returns total time since the sequence started. (Counter runs during PAUSE)
//   .remainingTotal()     → Returns remaining time until the full sequence completes. (Stops during PAUSE)
// 
//    .start(
//      Callback startCallback,   // executed at the beginning of each cycle
//      uint32_t on_ms,           // duration of ON phase
//      Callback endCallback,     // executed at the end of ON phase
//      uint32_t wait_ms,         // duration of WAIT phase
//      uint32_t repeatCount,     // number of cycles (0 = infinite)
//      Callback finishCallback   // executed once when sequence finishes
//    );
//
// -----------------------------------------------------------------------------

BlinkTask blink1;
Led led;
int debug;


void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nStart BlinkTask Class demo\n\n");

    led.init(1, 2, true, "0", "6", "12", "18", "24", "31");

    debug = false;
    uint8_t test = 1;

  if (test == 1) {
// blink, repeat 10      
      blink1.start(   // stsrt
        ledon,        // start function
        200,          // wait time
        ledmid,       // end function
        500,          // wait time
        10,           // repeat number (0: loop)
        ledoff        // finish function
    );

  }


  if (test == 2) {
// non blocking delay      
      blink1.start(
        nullptr,
        5000,
        ledon,
        0,
        1,
        nullptr
    );

  }


  if (test == 3) {
// blink      
      blink1.start(
        ledon,
        100,
        ledoff,
        100,
        0,
        nullptr
    );

  }

    
  if (test == 4) {
// lambda    
      static uint8_t mode = 1;

      blink1.start(
        [&](){                      // capture all by reference
            led.state(mode);
            mode ++;
        },
        200,
        [](){ led.state(0); },
        500,
        5,
        [](){ led.off(); }
      );
    
  }


  if (test == 5) {
// lambda mutable      
      auto fade = [n = 0]() mutable {
        led.on(n);        // 0–31
        n = (n + 1) % 32; // belső állapot
      };

      blink1.start(
        fade,             // lambda saját állapottal
        100,
        nullptr,
        0,
        0,
        nullptr
    );
    
  }

  
}


void ledon() {
  led.on(31);
}
void ledmid() {
  led.on(12);
}
void ledoff() {
  led.off();
}

    
void loop() {
  
  BlinkTask::tickAll();

  if (debug) {
    Serial.print("phase=");
    Serial.print(blink1.phase());          // 0/1/2/3

    Serial.print("  elPhase=");
    Serial.print(blink1.elapsedPhase());   // ms

    Serial.print("  remPhase=");
    Serial.print(blink1.remainingPhase()); // ms

    Serial.print("  elTotal=");
    Serial.print(blink1.elapsedTotal());   // ms
    Serial.print("  remTotal=");
    Serial.println(blink1.remainingTotal()); // ms

    delay(250);
  }

}
