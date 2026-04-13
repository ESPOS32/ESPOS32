# ESP32 / ESP8266 Framework

Lightweight, modular framework for ESP32 and ESP8266 focused on web-based control, file management, and device administration.

---

## 🚀 Features

### 🌐 Network
- Access Point mode
- Captive portal
- STA mode (Static IP / DHCP)
- WiFi extender mode
- Setup HTML interface
- Network state LED indicator

---

### 📁 LittleFS File Manager
- File editing (in-browser)
- Rename files
- Delete files
- Create folder
- Download files
- DoubleClick open
- Folder ⇄ `.tar` archive support
- Full filesystem backup / restore (`fullFS.tar`)

---

### 🧩 Core Classes
- Task Manager (lightweight scheduler)
- Neopixel LED driver
- Bi-color (2-pin) LED driver
- PWM LED driver
- Serial debug interface

---

### ⚙️ Custom UI
- Admin menu
- Setup menu
- Login screen
- Access denied screen
- Reboot screen

---

## 📦 Dependencies
- Only: **ESPAsyncWebServer**

---

## 🎯 Design Goals
- Minimal dependencies
- Fast and responsive web UI
- Compatible with constrained devices
- Easy integration into existing projects
- Clean and modular architecture

---

## 🛠️ Target Platforms
- ESP8266
- ESP32

---

## ▶️ Getting Started

```cpp
/*
 dependencies: 
  https://github.com/ESP32Async
   AsyncTCP (ESP32) / ESPAsyncTCP (ESP8266)
   ESPAsyncWebServer (WebResponseImpl.h first line: #define TEMPLATE_PLACEHOLDER '~' !!!
 */

// First boot: AP captive portal, IP:192.168.4.1

//#define EXTENDER 0     // disable WiFi range extender mode (reducing RAM usage)
#include "espos_core.h"

void setup() {

  FW = "alpha_v032";       // Firmware version
//  coreFolder = false;    // hide /core folder
//  systemFolder = false;  // hide /system folder
//  isDebug = false;       // serial debug off
  isLogged = true;         // Admin password not required
  
  wifiLed.init(1, 2, true, "0", "31", "31", "", "", "");          // LED (mode, pin, Vcc, state's) ESP12F built in led
//  wifiLed.init(2, 8, "000000", "FF0000", "0000FF", "", "", "");  // WS2812 LED  (mode, pin, state's) ESP32C3 devkit built in led
//  wifiLed.init(3, 4, 5, "0", "1", "31", "0", "0", "0");           // bicolor LED  (mode. pin1, pin2,  state's)
  
  initCPU(115200);         // serial baudrate, 
  
  initFS(false, 5, 3);     // true:force format, format after this many failed boots (0:off), boot end delay (sec)
  initWifi(0);             // 0:setup 1:AP 2:STA 3:EXT

  server->on("/raw", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", init_setup_html, wifiProcessor);
    });
    
  initRoutes();

  bootFinish();


//--------------------------------------------------------------user stuff----------------------------------------------------------------


  DEBUG("Admin Password: ", 1);   // Serial.print(F("text")) 
  DEBUG(passwd.admin);            // Serial.println
  DEBUG(ram());                   // Free RAM

}

void loop() {
  coreLoop();
}
