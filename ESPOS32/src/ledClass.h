#pragma once
#include <Arduino.h>

//-------------------------------------------------------NEOPIXEL LED------------------------------------------------------------

// --------- GAMMA IN FLASH ---------
static const uint8_t gamma8[256] PROGMEM = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   2,   3,
    3,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5,   5,   6,
    6,   6,   6,   7,   7,   7,   8,   8,   8,   9,   9,   9,   10,  10,  10,
    11,  11,  11,  12,  12,  13,  13,  13,  14,  14,  15,  15,  16,  16,  17,
    17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  22,  23,  24,  24,  25,
    25,  26,  27,  27,  28,  29,  29,  30,  31,  31,  32,  33,  34,  34,  35,
    36,  37,  38,  38,  39,  40,  41,  42,  42,  43,  44,  45,  46,  47,  48,
    49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
    64,  65,  66,  68,  69,  70,  71,  72,  73,  75,  76,  77,  78,  80,  81,
    82,  84,  85,  86,  88,  89,  90,  92,  93,  94,  96,  97,  99,  100, 102,
    103, 105, 106, 108, 109, 111, 112, 114, 115, 117, 119, 120, 122, 124, 125,
    127, 129, 130, 132, 134, 136, 137, 139, 141, 143, 145, 146, 148, 150, 152,
    154, 156, 158, 160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180, 182,
    184, 186, 188, 191, 193, 195, 197, 199, 202, 204, 206, 209, 211, 213, 215,
    218, 220, 223, 225, 227, 230, 232, 235, 237, 240, 242, 245, 247, 250, 252,
    255};

inline uint8_t GC(uint8_t x) {
  return pgm_read_byte(&gamma8[x]);
}
// --------- STRIP STATE (GRB ONE BUFFER) ---------
struct Strip {
  bool initialized;
  uint16_t count;
  uint8_t* grb;   // size = count * 3
};
static Strip strips[40];

static bool ensure_strip(uint8_t pin, uint16_t num) {

  Strip &s = strips[pin];

  if (!s.initialized) {
    s.grb = (uint8_t*)malloc(num * 3);
    if (!s.grb) return false;
    s.count = num;
    s.initialized = true;
  }
  return true;
}

#if defined(ESP32)

#include <driver/rmt_tx.h>
#include <driver/rmt_encoder.h>

static void neopix_flush(uint8_t pin) {
    Strip &s = strips[pin];
    if (!s.initialized) return;

    static bool init = false;
    static rmt_channel_handle_t channel = NULL;
    static rmt_encoder_handle_t bytes_encoder = NULL;

    if (!init) {

        // --- 1. TX csatorna létrehozása ---
        rmt_tx_channel_config_t tx_chan_config = {};   // <<< minden mező 0
        tx_chan_config.gpio_num = (gpio_num_t)pin;
        tx_chan_config.clk_src = RMT_CLK_SRC_DEFAULT;
        tx_chan_config.resolution_hz = 40'000'000;     // 25 ns tick
        tx_chan_config.mem_block_symbols = 64;
        tx_chan_config.trans_queue_depth = 4;
        tx_chan_config.intr_priority = 0;

        rmt_new_tx_channel(&tx_chan_config, &channel);

        // --- 2. Bytes encoder (WS2812 bitidőkkel) ---
        rmt_bytes_encoder_config_t bytes_cfg = {};

        // 0-bit (T0H + T0L)
        bytes_cfg.bit0.duration0 = 14;   // 14 * 25ns = 350ns
        bytes_cfg.bit0.level0    = 1;
        bytes_cfg.bit0.duration1 = 36;   // 36 * 25ns = 900ns   <<< FIX
        bytes_cfg.bit0.level1    = 0;

        // 1-bit (T1H + T1L)
        bytes_cfg.bit1.duration0 = 28;   // 700ns
        bytes_cfg.bit1.level0    = 1;
        bytes_cfg.bit1.duration1 = 24;   // 600ns
        bytes_cfg.bit1.level1    = 0;

        bytes_cfg.flags.msb_first = 1;

        rmt_new_bytes_encoder(&bytes_cfg, &bytes_encoder);

        // --- 3. Csatorna engedélyezése ---
        rmt_enable(channel);
        gpio_set_level((gpio_num_t)pin, 0);
        ets_delay_us(100);               // <<< stabil reset idő
        init = true;
    }

    // --- 4. Küldési konfiguráció ---
    rmt_transmit_config_t tx_cfg = {};   // <<< minden mező 0
    tx_cfg.loop_count = 0;

    // --- 5. GRB buffer kiküldése ---
    rmt_transmit(
        channel,
        bytes_encoder,
        s.grb,
        s.count * 3,
        &tx_cfg
    );

    // --- 6. Várakozás a TX befejezésére ---
    rmt_tx_wait_all_done(channel, portMAX_DELAY);

    // --- 7. Reset idő (WS2812B: 50µs min, ESP32-n 100µs stabil) ---
    ets_delay_us(100);
}

#elif defined(ESP8266)

static uint32_t IRAM_ATTR _getCycleCount(void) {
  uint32_t ccount;
  __asm__ __volatile__("rsr %0,ccount":"=a" (ccount));
  return ccount;
}

// WS2812 időzítések nanosec-ben
#define T0H_NS  350
#define T0L_NS  900
#define T1H_NS  700
#define T1L_NS  600

IRAM_ATTR static void sendByte(uint8_t byte, uint8_t pin) {

  // CPU frekvencia MHz-ben (80 vagy 160)
  uint32_t cpu = ESP.getCpuFreqMHz();   // 80 vagy 160

  // Ciklusidők kiszámítása:
  // ciklusok = (MHz * ns) / 1000
  uint32_t t0h = (cpu * T0H_NS) / 1000;
  uint32_t t0l = (cpu * T0L_NS) / 1000;
  uint32_t t1h = (cpu * T1H_NS) / 1000;
  uint32_t t1l = (cpu * T1L_NS) / 1000;

  uint32_t pinMask = _BV(pin);

  for (uint8_t mask = 0x80; mask; mask >>= 1) {

    uint32_t start = _getCycleCount();

    if (byte & mask) {
      // BIT = 1
      GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, pinMask);
      while ((_getCycleCount() - start) < t1h);
      GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, pinMask);
      while ((_getCycleCount() - start) < (t1h + t1l));
    } else {
      // BIT = 0
      GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, pinMask);
      while ((_getCycleCount() - start) < t0h);
      GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, pinMask);
      while ((_getCycleCount() - start) < (t0h + t0l));
    }
  }
}

static void neopix_flush(uint8_t pin) {
  Strip &s = strips[pin];
  if (!s.initialized) return;

  pinMode(pin, OUTPUT);
  noInterrupts();

  uint8_t* p = s.grb;
  for (uint16_t i = 0; i < s.count; i++) {
    sendByte(*p++, pin); // G
    sendByte(*p++, pin); // R
    sendByte(*p++, pin); // B
  }

  interrupts();
  delayMicroseconds(80);   // reset idő
}

#endif


//
// --------- PUBLIC API ---------
// true  → init (ha kell) + fill + flush
// false → modify + flush
//
inline void neopix(uint8_t pin, uint8_t r, uint8_t g, uint8_t b, uint16_t num, bool mode) {

  Strip &s = strips[pin];

  if (mode) {
    // FIRST TRUE → INIT
    if (!ensure_strip(pin, num)) return;

    uint8_t* p = s.grb;
    for (uint16_t i = 0; i < s.count; i++) {
      *p++ = g;
      *p++ = r;
      *p++ = b;
    }
  } else {
    // MODIFY ONLY
    if (!s.initialized) return;
    if (num >= s.count) return;

    uint8_t* p = s.grb + num * 3;
    p[0] = g;
    p[1] = r;
    p[2] = b;
  }

  neopix_flush(pin);
}

// -------------------------------------------------
//  parseHexColor("#ffAA00", r, g, b)
//  Returns true on success, false on invalid input.
// -------------------------------------------------
inline bool parseHexColor(const String &hex, uint8_t &r, uint8_t &g, uint8_t &b) 
{
    auto hexNibble = [](char c, uint8_t &out) -> bool {
        if (c >= '0' && c <= '9') { out = c - '0'; return true; }
        if (c >= 'a' && c <= 'f') { out = c - 'a' + 10; return true; }
        if (c >= 'A' && c <= 'F') { out = c - 'A' + 10; return true; }
        return false;
    };
    String s = hex;
    if (s.length() > 0 && s[0] == '#')
        s.remove(0, 1);
    if (s.length() != 6)
        return false;
    uint8_t n1, n2;
    if (!hexNibble(s[0], n1) || !hexNibble(s[1], n2)) return false;
    uint8_t rr = (n1 << 4) | n2;
    if (!hexNibble(s[2], n1) || !hexNibble(s[3], n2)) return false;
    uint8_t gg = (n1 << 4) | n2;
    if (!hexNibble(s[4], n1) || !hexNibble(s[5], n2)) return false;
    uint8_t bb = (n1 << 4) | n2;
    r = GC(rr);
    g = GC(gg);
    b = GC(bb);
    return true;
}

inline void neoLed(uint8_t port, String color) {
    uint8_t r, g, b;
    if (!parseHexColor(color, r, g, b)) return;
    neopix(port, r, g, b, 1, true);
}


//-----------------------------------------------------------------PWM LED----------------------------------------------------

#define MAX_PORT 17
#if defined(ESP32)
  #include "soc/gpio_struct.h"
  #include "driver/gpio.h"
  #define GPIO_READ()        GPIO.in.val
  #define GPIO_SET(mask)     GPIO.out_w1ts.val = (mask)
  #define GPIO_CLR(mask)     GPIO.out_w1tc.val = (mask)
#elif defined(ESP8266)
  #define GPIO_READ()        GPIO_REG_READ(GPIO_IN_ADDRESS)
  #define GPIO_SET(mask)     GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, (mask))
  #define GPIO_CLR(mask)     GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, (mask))
#endif

struct DLedPair {
  uint8_t inPin;
  uint8_t outPin;
};
DLedPair dLedPairs[MAX_PORT];
int dLedPairCount = 0;
uint32_t inMasks[MAX_PORT];
uint32_t outMasks[MAX_PORT];
volatile bool mirrorActive = false;
static bool isrAttachedForPin[MAX_PORT] = { false };

void removePairByInPin(uint8_t inPin) {
    noInterrupts();
    for (int i = 0; i < dLedPairCount; i++) {
        if (dLedPairs[i].inPin == inPin) {
            int last = dLedPairCount - 1;
            dLedPairs[i] = dLedPairs[last];
            inMasks[i]   = inMasks[last];
            outMasks[i]  = outMasks[last];
            dLedPairCount--;
            break;
        }
    }
    mirrorActive = (dLedPairCount > 0);
    interrupts();
}

void IRAM_ATTR mirrorISR() {
    if (!mirrorActive) return;
    uint32_t in = GPIO_READ();
    uint32_t setMask = 0;
    uint32_t clrMask = 0;
    for (int i = 0; i < dLedPairCount; i++) {
        uint32_t bit = -(!!(in & inMasks[i]));
        clrMask |= outMasks[i] & bit;
        setMask |= outMasks[i] & ~bit;
    }
    if (setMask) GPIO_SET(setMask);
    if (clrMask) GPIO_CLR(clrMask);
}

static const uint8_t gamma32[32] PROGMEM = {
    0, 1, 2, 4, 6, 9, 12, 16,
    20, 25, 30, 36, 42, 49, 56, 64,
    72, 81, 90, 100, 110, 121, 132, 144,
    156, 169, 182, 196, 210, 225, 240, 255
};

const uint8_t dualGamma[31] PROGMEM = {
    0,   1,   2,   4,   6,   9,  13,  18,
   24,  31,  40,  50,  62,  76,  92, 108,
  124, 140, 156, 172, 186, 198, 210, 220,
  229, 236, 242, 247, 251, 254, 255
};

static bool storedLed[MAX_PORT] = { false };
void pwmInit(uint8_t inPin, uint8_t outPin) {
  if (!storedLed[inPin]) {
    pinMode(inPin, OUTPUT);
    pinMode(outPin, OUTPUT);
#if defined(ESP32)    
    ledcAttach(inPin, 500, 8);
#endif
    storedLed[inPin] = true;
    }
}

void pwmWrite(uint8_t pin, uint8_t pwm) {
#if defined(ESP32)
  ledcWrite(pin, pwm);
#elif defined(ESP8266)
  analogWrite(pin, pwm);
#endif  
}

inline void pwmLed(uint8_t port, uint8_t value, bool vcc) {
  if (value > 31) value = 31;
  value = pgm_read_byte(&gamma32[value]);
  if (vcc) value = uint8_t(255 - value);
  pwmInit(port, port);
  pwmWrite(port, value);
}

inline void dualLed(uint8_t inPin, uint8_t outPin, uint8_t color) {

   pwmInit(inPin, outPin); 
 
  if (color > 0 and color < 100) {
    bool addPinISR = true;
    for (uint8_t i = 0; i < dLedPairCount; i++) {
      if (dLedPairs[i].inPin == inPin || dLedPairs[i].outPin == outPin) {
        addPinISR = false;
        break;
        }
      }
    if (addPinISR) {
      uint8_t i = dLedPairCount++;
      dLedPairs[i] = { inPin, outPin };
      inMasks[i]  = 1UL << inPin;
      outMasks[i] = 1UL << outPin;     
      mirrorActive = true;
      if (!isrAttachedForPin[inPin]) {
        attachInterrupt(digitalPinToInterrupt(inPin), mirrorISR, CHANGE);
        isrAttachedForPin[inPin] = true;
        }
      }
    if (color > 31) color = 31;
    color --;
    pwmWrite(inPin, pgm_read_byte(&dualGamma[color]));
    if (color == 0) digitalWrite(outPin, HIGH);  
    return;  
    }

  removePairByInPin(inPin);
    
  if (color > 99 and color < 200) {
    color -= 100;
    if (color > 31) color = 31;
    pwmWrite(inPin, 255 - pgm_read_byte(&gamma32[color]));
    digitalWrite(outPin, HIGH);
    return;
    }
    
  if (color > 199) {
    color -= 200;
    if (color > 31) color = 31;
    pwmWrite(inPin, pgm_read_byte(&gamma32[color]));
    digitalWrite(outPin, LOW);
    return; 
    }

  pwmWrite(inPin, 0);
  digitalWrite(outPin, LOW);
    
}
    



//--------------------------------------------------------------------LED CLASS---------------------------------------------------------

class Led {
public:
    Led() : mode(0), port(0), vcc(true), inPin(0), outPin(0) {}

    uint8_t getMode() const {
        return mode;
    }

    // MODE 2 – NeoPixel
    void init(uint8_t mode_, uint8_t port_,
              String s0, String s1, String s2, String s3, String s4, String s5) {

        mode = mode_;
        port = port_;
        states[0] = s0;
        states[1] = s1;
        states[2] = s2;
        states[3] = s3;
        states[4] = s4;
        states[5] = s5;
        state(0);
    }

    // MODE 1 – PWM
    // MODE 3 – Dual
    void init(uint8_t mode_, uint8_t a, uint8_t b,
              String s0, String s1, String s2, String s3, String s4, String s5) {

        mode = mode_;
        if (mode == 1) {
            port = a;
            vcc  = (b != 0);
        }
        else if (mode == 3) {
            inPin  = a;
            outPin = b;
        }
        states[0] = s0;
        states[1] = s1;
        states[2] = s2;
        states[3] = s3;
        states[4] = s4;
        states[5] = s5;
        state(0);
    }

    // --- STATE ---
    void state(uint8_t idx) {
        if (idx > 5) return;
        String &s = states[idx];
        if (mode == 2) {
            on(s);
        } else {
            on((uint8_t)s.toInt());
        }
    }
    // --- SET STATE ---
    void setState(uint8_t idx, String value) {
        if (idx > 5) return;
        states[idx] = value;
    }
    // --- ON OVERLOADOK ---
    void on(uint8_t value) {
        if (mode == 1) {
            pwmLed(port, value, vcc);
        }
        else if (mode == 3) {
            dualLed(inPin, outPin, value);
        }
    }
    void on(String color) {
        if (mode == 2) {
            neoLed(port, color);
        }
    }
    // --- OFF ---
    void off() {
        if (mode == 1) {
            on((uint8_t)0);
        }
        else if (mode == 2) {
            on(String("000000"));
        }
        else if (mode == 3) {
            on((uint8_t)0);
        }
    }

private:
    uint8_t mode;
    // PWM + NeoPixel
    uint8_t port;
    bool    vcc;
    // Dual LED
    uint8_t inPin;
    uint8_t outPin;
    // --- STATE STORAGE ---
    String states[6];
};
