#pragma once

#ifndef EXTENDER
#define EXTENDER 1
#endif

#include <Arduino.h>
#include <LittleFS.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
  #include <ESPAsyncWebServer.h>
#pragma GCC diagnostic pop
#include <DNSServer.h>
#include "coreClass.h"
#include "ledClass.h"
#if defined(ESP32)
  #include <WiFi.h>
  #include <AsyncTCP.h>
  #include <Preferences.h>
  #include "esp_wifi.h"
  #include "esp_system.h"
  #include "freertos/FreeRTOS.h"
  #include "freertos/timers.h"
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  #include <EEPROM.h>
 extern "C" {
  #include <user_interface.h>
  #include <lwip/etharp.h>
  #include <lwip/prot/ethernet.h>
 }
  #include <lwip/napt.h>
  #include <lwip/dns.h>
  #include <LwipDhcpServer.h>
  #define NAPT 1000
  #define NAPT_PORT 10
#endif

inline String FW = "ESPOS_core_v100";
static const char DEFPAGE[] = "/index.html";
static const char ADMINHTML[] = "/core/admin/admin.html";
static const char LOGINHTML[] = "/core/admin/login.html";
static const char REBOOTHTML[] = "/core/admin/reboot.html";
static const char DENIEDHTML[] = "/core/admin/denied.html";
static const char EDITHTML[] = "/core/admin/editor.html";
static const char SETUPHTML[] = "/core/setup/setup";

static const char STYLE_FILE[] = "/system/setup/style.txt";
static const char SETUP_PASSWD_FILE[] = "/system/setup/passwd.bin";
static const char SETUP_WIFI_FILE[] = "/system/setup/wifi.bin";

inline bool systemFolder = true;
inline bool coreFolder = true;
inline bool isDebug = true;
inline bool isLogged = false;
inline bool isBooting = true;

struct PasswdConfig {
  char admin[32];
  char html1u[32];
  char html1p[32];
  char html2u[32];
  char html2p[32];
  char html3u[32];
  char html3p[32];
  char html4u[32];
  char html4p[32];
  char ws[32];
};

inline String resetReason = "";
inline String rebootText;
inline String rebootURL;
inline String rebootTime;
inline String rebootButton;

#include "setupWiFi.h"
#include "fileSystem.h"
#include "raw_html.h"

inline BlinkTask rebootBlink;
inline AsyncWebServer* server = nullptr;
inline String chipID;
inline String chipModel;
inline uint8_t wifiMode = 0;
inline WifiConfig wifi;
inline PasswdConfig passwd;

inline bool passwdSave() {
    LittleFS.mkdir("/system/setup");
    File f = LittleFS.open(SETUP_PASSWD_FILE, "w");
    if (!f) return false;
    size_t written = f.write((uint8_t*)&passwd, sizeof(PasswdConfig));
    f.close();
    return (written == sizeof(PasswdConfig));
}

inline bool passwdLoad() {
    if (!LittleFS.exists(SETUP_PASSWD_FILE)) {
        memset(&passwd, 0, sizeof(PasswdConfig));
        passwdSave();
        return false;
    }
    File f = LittleFS.open(SETUP_PASSWD_FILE, "r");
    if (!f) {
        memset(&passwd, 0, sizeof(PasswdConfig));
        return false;
    }
    size_t readBytes = f.read((uint8_t*)&passwd, sizeof(PasswdConfig));
    f.close();
    if (readBytes != sizeof(PasswdConfig)) {
        memset(&passwd, 0, sizeof(PasswdConfig));
        passwdSave();
        return false;
    }
    return true;
}

inline String ram() {
  String s = "Free RAM: " + String(ESP.getFreeHeap()) + " Bytes, Fragmentation: ";
#ifdef ESP8266
  s += String(ESP.getHeapFragmentation()) + "%";
#else
  size_t f = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
  size_t l = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
  s += String(100 - (l * 100 / f)) + "%";
#endif
  return s;
}

inline String wifiStatusToString() {
    switch (WiFi.status()) {
        case WL_IDLE_STATUS:      return "IDLE";
        case WL_NO_SSID_AVAIL:    return "NO_SSID_AVAIL";
        case WL_SCAN_COMPLETED:   return "SCAN_COMPLETED";
        case WL_CONNECTED:        return "CONNECTED";
        case WL_CONNECT_FAILED:   return "CONNECT_FAILED";
        case WL_CONNECTION_LOST:  return "CONNECTION_LOST";
        case WL_DISCONNECTED:     return "DISCONNECTED";
        default:                  return "UNKNOWN";
    }
}


inline void handleDenied(AsyncWebServerRequest *r) {
    if (!(LittleFS.exists(DENIEDHTML))) {
      r->send(200, "text/html", denied_html);
      } else {
      r->send(LittleFS, DENIEDHTML, String());  
      }
}

inline void guard(AsyncWebServerRequest *r, std::function<void()> okHandler) {
    if (!isLogged) {
        handleDenied(r);
        return;
    }
    okHandler();
}

inline void handleAdmin(AsyncWebServerRequest *r) {
  if (passwd.admin[0] == '\0') {isLogged = true;}
  if (isLogged) {
    if (LittleFS.exists(ADMINHTML)) {     
      r->send(LittleFS, ADMINHTML, String());
      } else {
      r->send(200, "text/html", init_setup_html, wifiProcessor);
      }
    }
    if (!isLogged) {      
      if (LittleFS.exists(LOGINHTML)) {
        r->send(LittleFS, LOGINHTML, String());  
        } else {
        r->send(200, "text/html", login_html);
        }
    }
}

inline void handleAdminPost(AsyncWebServerRequest *r) {
    if (!r->hasParam("password", true)) {
        r->send(400, "text/plain", "Error!");
        return;
    }
    String pass = r->getParam("password", true)->value();
    if (pass == passwd.admin) {
      isLogged = true;
      handleAdmin(r);   
      } else {
      isLogged = false;
      handleDenied(r);
    }
}

inline void handleRoot(AsyncWebServerRequest *r) {
    if (LittleFS.exists(DEFPAGE)) {
      r->send(LittleFS, DEFPAGE, String());
      } else {
      handleAdmin(r);
      }
}

inline void handleLogout(AsyncWebServerRequest *r) {
    isLogged = false;
    r->redirect(DEFPAGE);
}

inline void handlePasswdsave(AsyncWebServerRequest *r) {
    const AsyncWebParameter *p;
    p = r->getParam("pass_admin", true); strlcpy(passwd.admin, p ? p->value().c_str() : "", sizeof(passwd.admin));
    p = r->getParam("pass_html1u", true); strlcpy(passwd.html1u, p ? p->value().c_str() : "", sizeof(passwd.html1u));
    p = r->getParam("pass_html1p", true); strlcpy(passwd.html1p, p ? p->value().c_str() : "", sizeof(passwd.html1p));
    p = r->getParam("pass_html2u", true); strlcpy(passwd.html2u, p ? p->value().c_str() : "", sizeof(passwd.html2u));
    p = r->getParam("pass_html2p", true); strlcpy(passwd.html2p, p ? p->value().c_str() : "", sizeof(passwd.html2p));
    p = r->getParam("pass_html3u", true); strlcpy(passwd.html3u, p ? p->value().c_str() : "", sizeof(passwd.html3u));
    p = r->getParam("pass_html3p", true); strlcpy(passwd.html3p, p ? p->value().c_str() : "", sizeof(passwd.html3p));
    p = r->getParam("pass_html4u", true); strlcpy(passwd.html4u, p ? p->value().c_str() : "", sizeof(passwd.html4u));
    p = r->getParam("pass_html4p", true); strlcpy(passwd.html4p, p ? p->value().c_str() : "", sizeof(passwd.html4p));
    p = r->getParam("pass_ws", true); strlcpy(passwd.ws, p ? p->value().c_str() : "", sizeof(passwd.ws));
    passwdSave();
    rebootText = "Password saved!";
    rebootURL = "";
    rebootTime = "";
    rebootButton = "";
    r->redirect("/reboot");
}

inline void handleReboot(AsyncWebServerRequest *r) {
    if (LittleFS.exists(REBOOTHTML)) {     
      r->send(LittleFS, REBOOTHTML, String(), false, wifiProcessor);
      } else {
      r->send(200, "text/html", reboot_html, wifiProcessor);
      }
    rebootBlink.start(nullptr, 1000, [](){ ESP.restart(); }, 0, 1, nullptr);
}

inline void handleSetup(AsyncWebServerRequest *r) {
    r->send(LittleFS, (String)SETUPHTML + ".html", String());
}
inline void handleSetupWifi(AsyncWebServerRequest *r) {
    r->send(LittleFS, (String)SETUPHTML + "_wifi.html", String(), false, wifiProcessor);
}

inline void initCPU(uint32_t br) {
  Serial.begin(br);
  EEPROM.begin(512);
#if defined(ESP32)
    chipID = String((uint32_t)(ESP.getEfuseMac() >> 32), HEX);
    chipModel = String(ESP.getChipModel());
    esp_reset_reason_t r = esp_reset_reason();
    switch (r) {
      case ESP_RST_POWERON:   resetReason = F("Power-on reset"); break;
      case ESP_RST_EXT:       resetReason = F("External reset (EN pin)"); break;
      case ESP_RST_SW:        resetReason = F("Software reset"); break;
      case ESP_RST_PANIC:     resetReason = F("Panic / Guru Meditation"); break;
      case ESP_RST_INT_WDT:   resetReason = F("Interrupt watchdog"); break;
      case ESP_RST_TASK_WDT:  resetReason = F("Task watchdog"); break;
      case ESP_RST_WDT:       resetReason = F("Other watchdog"); break;
      case ESP_RST_DEEPSLEEP: resetReason = F("Wake from deep sleep"); break;
      case ESP_RST_BROWNOUT:  resetReason = F("Brownout"); break;
      case ESP_RST_SDIO:      resetReason = F("SDIO reset"); break;
      default:                resetReason = F("Unknown"); break;
    }    
#elif defined(ESP8266)
    chipID = String(ESP.getChipId(), HEX);
    chipModel = "ESP8266";
    rst_info *info = ESP.getResetInfoPtr();
    switch (info->reason) {
      case REASON_DEFAULT_RST:      resetReason = F("Power-on reset"); break;
      case REASON_WDT_RST:          resetReason = F("Hardware watchdog"); break;
      case REASON_EXCEPTION_RST:    resetReason = F("Exception"); break;
      case REASON_SOFT_WDT_RST:     resetReason = F("Software watchdog"); break;
      case REASON_SOFT_RESTART:     resetReason = F("Software restart"); break;
      case REASON_DEEP_SLEEP_AWAKE: resetReason = F("Wake from deep sleep"); break;
      case REASON_EXT_SYS_RST:      resetReason = F("External reset"); break;
      default:                      resetReason = F("Unknown"); break;
    }
  if (isDebug) delay(200);
#endif
    chipID.toUpperCase();
    DEBUG("\n\n-----------------BOOT START-------------------");
    DEBUG("| CPU Model: " + chipModel);
    DEBUG("|   Clock: " + String(ESP.getCpuFreqMHz()) + "MHz");
    DEBUG("|   Free RAM: " + String(ESP.getFreeHeap()/1024) + "kB");
    DEBUG("|   ChipID: " + chipID);
    DEBUG("|   FW version: " + FW);
    DEBUG("|   Reset reason: " + resetReason);
    DEBUG(debugSeparator);  
}



inline void initRoutes() {
    server->serveStatic("/", LittleFS, "/");//.setTemplateProcessor(wifiProcessor);
    server->onNotFound(handleRoot);
    server->on("/", HTTP_GET, handleRoot);
    server->on("/list", HTTP_POST, [](AsyncWebServerRequest *r){guard(r, [&](){ handleList(r); });});
    server->on("/list_s", HTTP_POST, [](AsyncWebServerRequest *r){guard(r, [&](){ handleListSafe(r); });});
    server->on("/flashfree", HTTP_GET, [](AsyncWebServerRequest *r){guard(r, [&](){ handleFlashFree(r); });});
    server->on("/download", HTTP_POST, [](AsyncWebServerRequest *r){guard(r, [&](){ handleDownload(r); });});
    server->on("/format", HTTP_POST, [](AsyncWebServerRequest *r){guard(r, [&](){ handleFormat(r); });});
    server->on("/default", HTTP_POST, [](AsyncWebServerRequest *r){guard(r, [&](){ handleDefault(r); });});
    server->on("/saveall", HTTP_POST, [](AsyncWebServerRequest *r){guard(r, [&](){ handleSaveAll(r); });});
    server->on("/mkdir", HTTP_POST, [](AsyncWebServerRequest *r){guard(r, [&](){ handleMkdir(r); });});
    server->on("/delete", HTTP_POST, [](AsyncWebServerRequest *r){guard(r, [&](){ handleDelete(r); });});
    server->on("/rename", HTTP_POST, [](AsyncWebServerRequest *r){guard(r, [&](){ handleRename(r); });});
    server->on("/wifisave", HTTP_POST, [](AsyncWebServerRequest *r){guard(r, [&](){ handleWifisave(r); });});
    server->on("/passwdsave", HTTP_POST, [](AsyncWebServerRequest *r){guard(r, [&](){ handlePasswdsave(r); });});
    server->on("/upload", HTTP_POST, [](AsyncWebServerRequest *r){ r->send(200); }, NULL, handleUpload);
    server->on("/reboot", HTTP_GET, [](AsyncWebServerRequest *r){guard(r, [&](){ handleReboot(r); });});
    server->on("/edit", HTTP_POST, [](AsyncWebServerRequest *r){guard(r, [&](){ handleEditor(r); });});
    server->on("/admin", HTTP_POST, handleAdminPost);
    server->on("/admin", HTTP_GET, handleAdmin);
    server->on("/logout", HTTP_GET, handleLogout);
    server->on("/style.js", HTTP_GET, handleStyle);
    
    server->on("/setup", HTTP_GET, [](AsyncWebServerRequest *r){guard(r, [&](){ handleSetup(r); });});
    server->on("/setup_wifi", HTTP_GET, [](AsyncWebServerRequest *r){guard(r, [&](){ handleSetupWifi(r); });});
    
    server->addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
    server->begin();
}

inline void bootFinish() {
  ResetCounter::clear();
  DEBUG("|\n ----------------BOOT END.------------------\n");
  isBooting = false;
}


inline void coreLoop() {
  if (wifi.mode == 1 and wifi.ap.captive == true) {dnsServer.processNextRequest();}
  BlinkTask::tickAll();
}
