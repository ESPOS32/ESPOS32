#include <espos_core.h>
// -----------------------------------------------------------------------------
// Example: 16‑LED NeoPixel ring animation
//   - background: dark color wheel (dim rainbow)
//   - foreground: a single highlight pixel that brightens the color underneath
//
// This example demonstrates the usage of the neopix() API:
//
//   neopix(pin, r, g, b, index, true)
//       → on the first call: initializes the strip and fills all LEDs
//
//   neopix(pin, r, g, b, index, false)
//       → updates only one LED (by index), then flushes
//
// !!!!! WARNING !!!!! DO NOT USE !!!!!
//   ESP8266: GPIO6, GPIO7, GPIO8, GPIO9, GPIO10, GPIO11
//   ESP32C3: GPIO12, GPIO13, GPIO14, GPIO15, GPIO16, GPIO17
//
// -----------------------------------------------------------------------------

const uint8_t PIN = 14;   // NeoPixel data pin
const uint8_t N   = 16;   // Number of LEDs

// -----------------------------------------------------------------------------
// Simple color wheel (0–255 → RGB)
// -----------------------------------------------------------------------------
void colorWheel(uint8_t pos, uint8_t &r, uint8_t &g, uint8_t &b) {
  if (pos < 85) {
    r = pos * 3;
    g = 255 - pos * 3;
    b = 0;
  } else if (pos < 170) {
    pos -= 85;
    r = 255 - pos * 3;
    g = 0;
    b = pos * 3;
  } else {
    pos -= 170;
    r = 0;
    g = pos * 3;
    b = 255 - pos * 3;
  }
}

// -----------------------------------------------------------------------------
// Setup: initialize the strip
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting dark color wheel + highlight pixel animation"));

  // The first TRUE call to neopix():
  //   - allocates the GRB buffer
  //   - sets the LED count
  //   - fills all LEDs with the given color
  //   - flushes the output
  neopix(PIN, 0, 0, 0, N, true);   // entire ring set to black
}

// -----------------------------------------------------------------------------
// Loop: background + highlight pixel animation
// -----------------------------------------------------------------------------
void loop() {

  static uint8_t offset = 0;   // rotation offset for the background color wheel
  static uint8_t pos    = 0;   // position of the highlight pixel

  // ---------------------------------------------------------------------------
  // 1) Background: dark color wheel
  //    Each LED receives a dim rainbow color.
  //    The background is recolored every cycle.
  // ---------------------------------------------------------------------------
  for (uint8_t i = 0; i < N; i++) {

    uint8_t r, g, b;
    colorWheel((i * 16 + offset), r, g, b);

    // Darken the background (dim rainbow)
    r >>= 3;
    g >>= 3;
    b >>= 3;

    // Modify a single LED (index = i)
    // mode = false → update only, no initialization
    neopix(PIN, r, g, b, i, false);
  }

  // ---------------------------------------------------------------------------
  // 2) Highlight pixel:
  //    The running pixel brightens the color underneath it.
  // ---------------------------------------------------------------------------
  {
    uint8_t r, g, b;
    colorWheel((pos * 16 + offset), r, g, b);

    // Brighten (double intensity, capped at 255)
    r = min(255, r << 1);
    g = min(255, g << 1);
    b = min(255, b << 1);

    // The highlight pixel overrides the background color
    neopix(PIN, r, g, b, pos, false);
  }

  // ---------------------------------------------------------------------------
  // 3) Update positions
  // ---------------------------------------------------------------------------
  pos++;
  if (pos >= N) pos = 0;

  offset++;   // slowly rotate the background color wheel

  delay(40);  // ~25 FPS
}
