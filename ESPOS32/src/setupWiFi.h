#pragma once
#include "espos_core.h"
#include "wifiConfig.h"
#include "raw_html.h"

extern WifiConfig wifi;
extern uint8_t wifiMode;
extern PasswdConfig passwd;
extern AsyncWebServer *server;
String WifiModeStr();
static int _apClientCount = 0;
static DNSServer dnsServer;

Led wifiLed;
BlinkTask wifiBlink;


inline void onApClientJoined(const String& mac) {
  if (!isBooting) DEBUG("AP client joined: " + mac);
  if (!wifiLed.getMode()) return;
  
  wifiBlink.start(
  [](){ wifiLed.state(1); },
  75,
  [](){ wifiLed.state(0); },
  500,
  0,
  [](){ wifiLed.state(0); }
  );
}

inline void onApClientLeft(const String& mac) {
  if (!isBooting) DEBUG("AP client disconnected: " + mac);
}

inline void onApLastClientLeft() {
  if (!isBooting) DEBUG("AP last client left");
  if (!wifiLed.getMode()) return;
  wifiBlink.stop();
}

inline void onStaConnected(const String& ssid) {
  if (!isBooting) DEBUG("STA connected to " + ssid);
  if (!wifiLed.getMode()) return;
  
  if (wifiLed.getMode() == 1 and wifi.mode == 3) {
    wifiLed.setState(0, "3");
    wifiLed.state(0);
    return;
    }
  if (wifiLed.getMode() == 2 and wifi.mode == 3) {
    wifiLed.setState(0, "003300");
    wifiLed.state(0);
    return;
    }
  if (wifiLed.getMode() == 3 and wifi.mode == 3) {
    wifiLed.setState(0, "205");
    wifiLed.state(0);
    return;
    }

  wifiBlink.start(
  [](){ wifiLed.state(2); },
  75,
  [](){ wifiLed.state(0); },
  1500,
  0,
  nullptr
  );
  
}

inline void onStaDisconnected(int reason) {
  if (!isBooting) DEBUG("STA disconnected, reason=%d\n", reason);
  if (!wifiLed.getMode()) return;
  
  if (wifi.mode == 3) {
    wifiLed.setState(0, "000000");
    wifiLed.state(0);
    } else {
    wifiBlink.stop();
    } 
}


inline void WifiEventsInit() {

#if defined(ESP32)

  static WiFiEventId_t 
    h1 __attribute__((unused)),
    h2 __attribute__((unused)),
    h3 __attribute__((unused)),
    h4 __attribute__((unused));

  // AP: kliens csatlakozott
  h1 = WiFi.onEvent(
    [](WiFiEvent_t, WiFiEventInfo_t info) {
      char mac[18];
      sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X",
              info.wifi_ap_staconnected.mac[0], info.wifi_ap_staconnected.mac[1],
              info.wifi_ap_staconnected.mac[2], info.wifi_ap_staconnected.mac[3],
              info.wifi_ap_staconnected.mac[4], info.wifi_ap_staconnected.mac[5]);

      _apClientCount++;
      onApClientJoined(String(mac));
    },
    ARDUINO_EVENT_WIFI_AP_STACONNECTED
  );

  // AP: kliens lecsatlakozott
  h2 = WiFi.onEvent(
    [](WiFiEvent_t, WiFiEventInfo_t info) {
      char mac[18];
      sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X",
              info.wifi_ap_stadisconnected.mac[0], info.wifi_ap_stadisconnected.mac[1],
              info.wifi_ap_stadisconnected.mac[2], info.wifi_ap_stadisconnected.mac[3],
              info.wifi_ap_stadisconnected.mac[4], info.wifi_ap_stadisconnected.mac[5]);

      if (_apClientCount > 0) _apClientCount--;
      onApClientLeft(String(mac));
      if (_apClientCount == 0) onApLastClientLeft();
    },
    ARDUINO_EVENT_WIFI_AP_STADISCONNECTED
  );

  // STA: csatlakozott
  h3 = WiFi.onEvent(
    [](WiFiEvent_t, WiFiEventInfo_t) {
      onStaConnected(WiFi.SSID());
    },
    ARDUINO_EVENT_WIFI_STA_CONNECTED
  );

  // STA: leszakadt
  h4 = WiFi.onEvent(
    [](WiFiEvent_t, WiFiEventInfo_t info) {
      onStaDisconnected(info.wifi_sta_disconnected.reason);
    },
    ARDUINO_EVENT_WIFI_STA_DISCONNECTED
  );

#elif defined(ESP8266)

  static WiFiEventHandler h1, h2, h3, h4, h5;

  // AP: kliens csatlakozott
  h1 = WiFi.onSoftAPModeStationConnected(
    [](const WiFiEventSoftAPModeStationConnected& e) {
      char mac[18];
      sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X",
              e.mac[0], e.mac[1], e.mac[2],
              e.mac[3], e.mac[4], e.mac[5]);

      _apClientCount++;
      onApClientJoined(String(mac));
    }
  );

  // AP: kliens lecsatlakozott
  h2 = WiFi.onSoftAPModeStationDisconnected(
    [](const WiFiEventSoftAPModeStationDisconnected& e) {
      char mac[18];
      sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X",
              e.mac[0], e.mac[1], e.mac[2],
              e.mac[3], e.mac[4], e.mac[5]);

      if (_apClientCount > 0) _apClientCount--;
      onApClientLeft(String(mac));          // ÚJ
      if (_apClientCount == 0) onApLastClientLeft();
    }
  );

  // STA: csatlakozott (SSID itt megvan)
  h3 = WiFi.onStationModeConnected(
    [](const WiFiEventStationModeConnected& e) {
      onStaConnected(String(e.ssid));       // SSID átadása
    }
  );

  // STA: leszakadt
  h4 = WiFi.onStationModeDisconnected(
    [](const WiFiEventStationModeDisconnected& e) {
      onStaDisconnected(e.reason);
    }
  );

  // STA: IP-t kapott (nem kell külön callback)
  h5 = WiFi.onStationModeGotIP(
    [](const WiFiEventStationModeGotIP&) {
      // már connected, nem kell külön esemény
    }
  );
  
#endif
}


#if(EXTENDER)
#if defined(ESP32)
inline void onEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      WiFi.AP.enableNAPT(true);
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      WiFi.AP.enableNAPT(false);
      break;
    default:
      break;
  }
}
#elif defined(ESP8266)
static err_t arp_proxy(struct pbuf *p, struct netif *netif) {
  struct eth_hdr *ethhdr = (struct eth_hdr *)p->payload;
  if (ethhdr->type == PP_HTONS(ETHTYPE_ARP)) {
    etharp_input(p, netif);
    return ERR_OK;
  }
  return ERR_OK;
}
#endif
#endif

void handleReboot(AsyncWebServerRequest *r);
String wifiStatusToString();



// GLOBAL INSTANCE
//extern WifiConfig wifi;



// =======================
//  INIT (DEFAULT + FS)
// =======================

inline String wifiProcessor(const String& var) {

    // MODE
    if (var.equals(F("EXTENDER_ENABLE")))  return EXTENDER ? "" : "hidden";
    if (var.equals(F("MODE_AP")))          return (wifi.mode == USER_WIFI_AP)  ? "selected" : "";
    if (var.equals(F("MODE_STA")))         return (wifi.mode == USER_WIFI_STA) ? "selected" : "";
    if (var.equals(F("MODE_EXT")))         return (wifi.mode == USER_WIFI_EXT) ? "selected" : "";

    // AP
    if (var.equals(F("AP_SSID")))       return wifi.ap.ssid;
    if (var.equals(F("AP_PASS")))       return wifi.ap.password;
    if (var.equals(F("AP_CHANNEL")))    return String(wifi.ap.channel);
    if (var.equals(F("AP_HIDDEN_ON")))  return wifi.ap.hidden ? "selected" : "";
    if (var.equals(F("AP_HIDDEN_OFF"))) return wifi.ap.hidden ? "" : "selected";
    if (var.equals(F("AP_POWER")))      return String(wifi.ap.power);
    if (var.equals(F("AP_CAPTIVE_ON")))  return wifi.ap.captive ? "selected" : "";
    if (var.equals(F("AP_CAPTIVE_OFF"))) return wifi.ap.captive ? "" : "selected";
    if (var.equals(F("AP_DOMAIN")))      return wifi.ap.domain;

    if (var.equals(F("AP_IP0"))) return String(wifi.ap.ip.a);
    if (var.equals(F("AP_IP1"))) return String(wifi.ap.ip.b);
    if (var.equals(F("AP_IP2"))) return String(wifi.ap.ip.c);
    if (var.equals(F("AP_IP3"))) return String(wifi.ap.ip.d);
    if (var.equals(F("AP_PORT"))) return String(wifi.ap.port);

    if (var.equals(F("AP_HTTPS_ON")))  return wifi.ap.https ? "selected" : "";
    if (var.equals(F("AP_HTTPS_OFF"))) return wifi.ap.https ? "" : "selected";

    // STA
    if (var.equals(F("STA_SSID"))) return wifi.sta.ssid;
    if (var.equals(F("STA_PASS"))) return wifi.sta.password;
    if (var.equals(F("STA_DHCP_ON")))  return wifi.sta.dhcp ? "selected" : "";
    if (var.equals(F("STA_DHCP_OFF"))) return wifi.sta.dhcp ? "" : "selected";
    if (var.equals(F("STA_HTTPS_ON")))  return wifi.sta.https ? "selected" : "";
    if (var.equals(F("STA_HTTPS_OFF"))) return wifi.sta.https ? "" : "selected";

    if (var.equals(F("STA_IP0"))) return String(wifi.sta.ip.a);
    if (var.equals(F("STA_IP1"))) return String(wifi.sta.ip.b);
    if (var.equals(F("STA_IP2"))) return String(wifi.sta.ip.c);
    if (var.equals(F("STA_IP3"))) return String(wifi.sta.ip.d);

    if (var.equals(F("STA_GW0"))) return String(wifi.sta.gateway.a);
    if (var.equals(F("STA_GW1"))) return String(wifi.sta.gateway.b);
    if (var.equals(F("STA_GW2"))) return String(wifi.sta.gateway.c);
    if (var.equals(F("STA_GW3"))) return String(wifi.sta.gateway.d);

    if (var.equals(F("STA_M0"))) return String(wifi.sta.mask.a);
    if (var.equals(F("STA_M1"))) return String(wifi.sta.mask.b);
    if (var.equals(F("STA_M2"))) return String(wifi.sta.mask.c);
    if (var.equals(F("STA_M3"))) return String(wifi.sta.mask.d);

    if (var.equals(F("STA_D0"))) return String(wifi.sta.dns.a);
    if (var.equals(F("STA_D1"))) return String(wifi.sta.dns.b);
    if (var.equals(F("STA_D2"))) return String(wifi.sta.dns.c);
    if (var.equals(F("STA_D3"))) return String(wifi.sta.dns.d);

    if (var.equals(F("STA_PORT"))) return String(wifi.sta.port);

    // EXT
    if (var.equals(F("EXT_SSID")))        return wifi.ext.ssid;
    if (var.equals(F("EXT_PASS")))        return wifi.ext.password;
    if (var.equals(F("EXT_CONNECTED")))   return wifiStatusToString();
    if (var.equals(F("EXT_NEWSSID")))     return wifi.ext.newSSID;
    if (var.equals(F("EXT_NEWPASS")))     return wifi.ext.newPass;

    if (var.equals(F("EXT_HIDDEN_ON")))  return wifi.ext.hidden ? "selected" : "";
    if (var.equals(F("EXT_HIDDEN_OFF"))) return wifi.ext.hidden ? "" : "selected";

    if (var.equals(F("EXT_CHANNEL")))    return String(wifi.ext.channel);

    if (var.equals(F("EXT_IP0"))) return String(wifi.ext.ip.a);
    if (var.equals(F("EXT_IP1"))) return String(wifi.ext.ip.b);
    if (var.equals(F("EXT_IP2"))) return String(wifi.ext.ip.c);
    if (var.equals(F("EXT_IP3"))) return String(wifi.ext.ip.d);

    if (var.equals(F("EXT_M0"))) return String(wifi.ext.mask.a);
    if (var.equals(F("EXT_M1"))) return String(wifi.ext.mask.b);
    if (var.equals(F("EXT_M2"))) return String(wifi.ext.mask.c);
    if (var.equals(F("EXT_M3"))) return String(wifi.ext.mask.d);

    if (var.equals(F("EXT_D0"))) return String(wifi.ext.dns.a);
    if (var.equals(F("EXT_D1"))) return String(wifi.ext.dns.b);
    if (var.equals(F("EXT_D2"))) return String(wifi.ext.dns.c);
    if (var.equals(F("EXT_D3"))) return String(wifi.ext.dns.d);

    if (var.equals(F("EXT_S0"))) return String(wifi.ext.startip.a);
    if (var.equals(F("EXT_S1"))) return String(wifi.ext.startip.b);
    if (var.equals(F("EXT_S2"))) return String(wifi.ext.startip.c);
    if (var.equals(F("EXT_S3"))) return String(wifi.ext.startip.d);

    // MAC
    if (var.equals(F("MAC_MODE"))) return String(wifi.mac.macMode);

    if (var.equals(F("MAC0"))) return wifi.mac.customMac.m[0];
    if (var.equals(F("MAC1"))) return wifi.mac.customMac.m[1];
    if (var.equals(F("MAC2"))) return wifi.mac.customMac.m[2];
    if (var.equals(F("MAC3"))) return wifi.mac.customMac.m[3];
    if (var.equals(F("MAC4"))) return wifi.mac.customMac.m[4];
    if (var.equals(F("MAC5"))) return wifi.mac.customMac.m[5];

    // PASSWD
    if (var.equals(F("PWD_ADMIN"))) return passwd.admin;

    // REBOOT
    if (var.equals(F("REBOOT_TEXT")))   return rebootText;
    if (var.equals(F("REBOOT_URL")))    return rebootURL;
    if (var.equals(F("REBOOT_TIME")))   return rebootTime;
    if (var.equals(F("REBOOT_BUTTON"))) return rebootButton;

    return "";
}

inline void handleWifisave(AsyncWebServerRequest *r) {

    auto P = [&](const __FlashStringHelper *key) -> const String& {
        return r->getParam(key, true)->value();
    };

    uint8_t oldMode = wifi.mode;

    // MODE
    wifi.mode = (WifiUserMode) P(F("mode")).toInt();

    // AP
    strcpy(wifi.ap.ssid,     P(F("ap_ssid")).c_str());
    strcpy(wifi.ap.password, P(F("ap_pass")).c_str());

    wifi.ap.channel = P(F("ap_channel")).toInt();
    wifi.ap.power   = P(F("ap_power")).toInt();
    wifi.ap.hidden  = P(F("ap_hidden")).toInt();
    wifi.ap.captive = P(F("ap_captive")).toInt();

    strcpy(wifi.ap.domain, P(F("ap_domain")).c_str());

    wifi.ap.ip.a = P(F("ap_ip0")).toInt();
    wifi.ap.ip.b = P(F("ap_ip1")).toInt();
    wifi.ap.ip.c = P(F("ap_ip2")).toInt();
    wifi.ap.ip.d = P(F("ap_ip3")).toInt();

    wifi.ap.https = P(F("ap_https")).toInt();
    wifi.ap.port  = P(F("ap_port")).toInt();

    // STA
    strcpy(wifi.sta.ssid,     P(F("sta_ssid")).c_str());
    strcpy(wifi.sta.password, P(F("sta_pass")).c_str());
    wifi.sta.dhcp = P(F("sta_dhcp")).toInt();

    wifi.sta.ip.a = P(F("sta_ip0")).toInt();
    wifi.sta.ip.b = P(F("sta_ip1")).toInt();
    wifi.sta.ip.c = P(F("sta_ip2")).toInt();
    wifi.sta.ip.d = P(F("sta_ip3")).toInt();

    wifi.sta.gateway.a = P(F("sta_gw0")).toInt();
    wifi.sta.gateway.b = P(F("sta_gw1")).toInt();
    wifi.sta.gateway.c = P(F("sta_gw2")).toInt();
    wifi.sta.gateway.d = P(F("sta_gw3")).toInt();

    wifi.sta.mask.a = P(F("sta_m0")).toInt();
    wifi.sta.mask.b = P(F("sta_m1")).toInt();
    wifi.sta.mask.c = P(F("sta_m2")).toInt();
    wifi.sta.mask.d = P(F("sta_m3")).toInt();

    wifi.sta.dns.a = P(F("sta_d0")).toInt();
    wifi.sta.dns.b = P(F("sta_d1")).toInt();
    wifi.sta.dns.c = P(F("sta_d2")).toInt();
    wifi.sta.dns.d = P(F("sta_d3")).toInt();

    wifi.sta.https = P(F("sta_https")).toInt();
    wifi.sta.port  = P(F("sta_port")).toInt();

    // EXT
    strcpy(wifi.ext.ssid,     P(F("ext_ssid")).c_str());
    strcpy(wifi.ext.password, P(F("ext_pass")).c_str());
    strcpy(wifi.ext.newSSID,  P(F("ext_newssid")).c_str());
    strcpy(wifi.ext.newPass,  P(F("ext_newpass")).c_str());

    wifi.ext.hidden  = P(F("ext_hidden")).toInt();
    wifi.ext.channel = P(F("ext_channel")).toInt();

    wifi.ext.ip.a = P(F("ext_ip0")).toInt();
    wifi.ext.ip.b = P(F("ext_ip1")).toInt();
    wifi.ext.ip.c = P(F("ext_ip2")).toInt();
    wifi.ext.ip.d = P(F("ext_ip3")).toInt();

    wifi.ext.mask.a = P(F("ext_m0")).toInt();
    wifi.ext.mask.b = P(F("ext_m1")).toInt();
    wifi.ext.mask.c = P(F("ext_m2")).toInt();
    wifi.ext.mask.d = P(F("ext_m3")).toInt();

    wifi.ext.dns.a = P(F("ext_d0")).toInt();
    wifi.ext.dns.b = P(F("ext_d1")).toInt();
    wifi.ext.dns.c = P(F("ext_d2")).toInt();
    wifi.ext.dns.d = P(F("ext_d3")).toInt();

    wifi.ext.startip.a = P(F("ext_s0")).toInt();
    wifi.ext.startip.b = P(F("ext_s1")).toInt();
    wifi.ext.startip.c = P(F("ext_s2")).toInt();
    wifi.ext.startip.d = P(F("ext_s3")).toInt();

    // MAC (heap‑mentes verzió)
    wifi.mac.macMode = P(F("mac_mode")).toInt();

    for (int i = 0; i < 6; i++) {
        char key[6];
        snprintf(key, sizeof(key), "mac%d", i);
        const String &v = r->getParam(key, true)->value();
        strncpy(wifi.mac.customMac.m[i], v.c_str(), 3);
        wifi.mac.customMac.m[i][2] = '\0';
    }

    saveWifiConfig();

    rebootText   = "";
    rebootURL    = "";
    if (oldMode != wifi.mode) {
      rebootTime   = "30000";
      rebootButton = "I conneted";
      } else {
      rebootTime   = "8000";  
      }

    r->redirect("/reboot");
}


inline uint8_t findCleanestChannel(uint8_t ch) {
    // 1–15 → fix csatorna
    if (ch >= 1 && ch <= 15) {
        return ch;
    }

    // régió meghatározása
    uint8_t maxCh;
    if (ch == 16) maxCh = 13;   // EU
    else if (ch == 17) maxCh = 11; // USA
    else return 1; // érvénytelen bemenet → fallback

    float score[16] = {0};  // 1–15 tartomány

    int n = WiFi.scanNetworks(false, true);

    for (int i = 0; i < n; i++) {
        uint8_t c = WiFi.channel(i);
        if (c < 1 || c > maxCh) continue;

        int rssi = abs(WiFi.RSSI(i));  // erősebb jel → nagyobb interferencia
        float w = rssi / 10.0f;        // normalizált súly (pl. -40 → 4.0)

        // saját csatorna
        score[c] += w;

        // szomszédos csatornák
        if (c > 1)     score[c - 1] += w * 0.5f;
        if (c < maxCh) score[c + 1] += w * 0.5f;

        // 2 csatornával arrébb
        if (c > 2)     score[c - 2] += w * 0.2f;
        if (c < maxCh - 1) score[c + 2] += w * 0.2f;
    }

    // legkisebb interferenciájú csatorna kiválasztása
    uint8_t bestCh = 1;
    float bestScore = score[1];

    for (uint8_t c = 2; c <= maxCh; c++) {
        if (score[c] < bestScore) {
            bestScore = score[c];
            bestCh = c;
        }
    }

    return bestCh;  // 1–11 vagy 1–13
}

inline String setupMac(const WifiMACConfig &cfg, WifiUserMode mode) {
    uint8_t macAP[6];
    uint8_t macSTA[6];
    const char *modeText = "Original";

    auto setBoth = [&](uint8_t *src) {
#if defined(ESP32)
      esp_wifi_set_mac(WIFI_IF_AP,  src);
      esp_wifi_set_mac(WIFI_IF_STA, src);
#elif defined(ESP8266)
      wifi_set_macaddr(SOFTAP_IF,  src);
      wifi_set_macaddr(STATION_IF, src);
#endif

    };

    // RANDOM MAC
    if (cfg.macMode == 1) {
        for (int i = 0; i < 6; i++) macAP[i] = macSTA[i] = random(0, 255);
        setBoth(macAP);
        modeText = "Random";
    }

    // CUSTOM MAC
    else if (cfg.macMode == 2) {
        for (int i = 0; i < 6; i++)
            macAP[i] = macSTA[i] = strtoul(cfg.customMac.m[i], nullptr, 16);
        setBoth(macAP);
        modeText = "Custom";
    }

    // DEFAULT MAC
    else {
#if defined(ESP32)
      esp_wifi_get_mac(WIFI_IF_STA, macSTA);
      esp_wifi_get_mac(WIFI_IF_AP,  macAP);
#elif defined(ESP8266)
      wifi_get_macaddr(STATION_IF, macSTA);
      wifi_get_macaddr(SOFTAP_IF,  macAP);
#endif
    }

    // Formázó lambda
    auto fmt = [&](uint8_t m[6], const char *label) {
        char buf[64];
        snprintf(buf, sizeof(buf),
                 "|   MAC %s: %02X-%02X-%02X-%02X-%02X-%02X (%s)",
                 label,
                 m[0], m[1], m[2], m[3], m[4], m[5],
                 modeText);
        return String(buf);
    };

    // Visszatérés módonként
    if (mode == USER_WIFI_AP)
        return fmt(macAP, "AP");

    if (mode == USER_WIFI_STA)
        return fmt(macSTA, "STA");

    return fmt(macAP, "AP ") + "\n" + fmt(macSTA, "STA");
}

inline bool connectRouter(String wmode) {
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(500);yield();
    DEBUG(".", 1);
    }
  if (WiFi.status() == WL_CONNECTED) {
    DEBUG(" Connected");
    DEBUG("|   Channel: ", 1);
    DEBUG(WiFi.channel());
    DEBUG("|   RSSI: ", 1);
    DEBUG(WiFi.RSSI(), 1);
    DEBUG("dBm");
    DEBUG("|   Mode: ", 1);
    DEBUG(wmode);
    return true;
    }
  wifiMode = 0;
  DEBUG(" Connect failed");
  return false;
}
  

inline uint8_t wifiStartAP() {
  
  WiFi.mode(WIFI_AP);
  String macInfo = setupMac(wifi.mac, wifi.mode);
#if defined(ESP32)
  esp_wifi_set_max_tx_power(82);
#elif defined(ESP8266)
  WiFi.setOutputPower(20.5f);
#endif
  WiFi.softAPConfig(toIP(wifi.ap.ip), toIP(wifi.ap.ip), IPAddress(255,255,255,0));
  WiFi.softAP(wifi.ap.ssid, wifi.ap.password, findCleanestChannel(wifi.ap.channel), wifi.ap.hidden, 4);
  dnsServer.start(53, "*", WiFi.softAPIP());
  DEBUG("|   SSID: ", 1); DEBUG(wifi.ap.ssid, 1); if (wifi.ap.hidden) DEBUG(" (Hidden)", 1);
  DEBUG("\n|   PASS: ", 1); DEBUG(wifi.ap.password);
  DEBUG("|   Channel: ", 1); DEBUG(WiFi.channel());
  DEBUG("|   Mode: Access Point", 1); if (wifi.ap.captive) {DEBUG(" Captive", 1);}
  DEBUG("");
  DEBUG(debugSeparator);  
  DEBUG("| Network:");
  DEBUG("|   IP: ", 1); DEBUG(WiFi.softAPIP());
  DEBUG("|   PORT: ", 1); DEBUG(wifi.ap.port);
  DEBUG(macInfo);
  wifiMode = 1;
  return wifiMode;
}

inline uint8_t wifiStartSTA() {
  DEBUG("|   Router SSID: ", 1); DEBUG(wifi.sta.ssid);
  DEBUG("|   Conneting...", 1);
  WiFi.mode(WIFI_STA);
  String macInfo = setupMac(wifi.mac, wifi.mode);
#if defined(ESP32)
  esp_wifi_set_max_tx_power(82);
#elif defined(ESP8266)
  WiFi.setOutputPower(20.5f);
#endif
  if (!wifi.sta.dhcp) {
    WiFi.config(toIP(wifi.sta.ip), toIP(wifi.sta.gateway), toIP(wifi.sta.mask), toIP(wifi.sta.dns));
    }
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(wifi.sta.ssid, wifi.sta.password);
  if (connectRouter("Client")) {
    wifiMode = 2;
    DEBUG(debugSeparator);  
    DEBUG("| Network:");
    DEBUG("|   IP: ", 1); DEBUG(WiFi.localIP());
    DEBUG("|   PORT: ", 1); DEBUG(wifi.sta.port);
    DEBUG("|   Netmask: ", 1); DEBUG(WiFi.subnetMask());
    DEBUG("|   Gateway: ", 1); DEBUG(WiFi.gatewayIP()); 
    DEBUG("|   DNS: ", 1); DEBUG(WiFi.dnsIP(0));
    DEBUG(macInfo);
    }
  return wifiMode;
}

#if(EXTENDER)
inline uint8_t wifiStartEXT() {
#if defined(ESP32)
  Network.onEvent(onEvent);
  WiFi.AP.begin();
  WiFi.AP.config(toIP(wifi.ext.ip), toIP(wifi.ext.ip), toIP(wifi.ext.mask), toIP(wifi.ext.startip), toIP(wifi.ext.dns));
  WiFi.AP.create(wifi.ext.newSSID, wifi.ext.newPass);
  WiFi.AP.waitStatusBits(ESP_NETIF_STARTED_BIT, 1000);
  wifi_config_t conf;
  esp_wifi_get_config(WIFI_IF_AP, &conf);
  conf.ap.ssid_hidden = wifi.ext.hidden;
  conf.ap.channel     = findCleanestChannel(wifi.ext.channel);
  esp_wifi_set_config(WIFI_IF_AP, &conf);
  esp_wifi_set_max_tx_power(80);
#elif defined(ESP8266)
  WiFi.mode(WIFI_AP_STA);
  WiFi.setOutputPower(20.5f);
#endif
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(wifi.ext.ssid, wifi.ext.password);
  String macInfo = setupMac(wifi.mac, wifi.mode);
  DEBUG("|   Source SSID: ", 1); DEBUG(wifi.ext.ssid);
  DEBUG("|   Connecting...", 1);
  connectRouter("Extender");
  wifiMode = 3;
#if defined(ESP8266)
  auto& serv = WiFi.softAPDhcpServer();
  serv.setDns(toIP(wifi.ext.dns));
  WiFi.softAPConfig(toIP(wifi.ext.ip), toIP(wifi.ext.ip), toIP(wifi.ext.mask));
  WiFi.softAP(wifi.ext.newSSID, wifi.ext.newPass, findCleanestChannel(wifi.ext.channel), wifi.ext.hidden);
  err_t ret = ip_napt_init(1000, 10);
  if (ret == ERR_OK) {
    ip_napt_enable_no(SOFTAP_IF, 1);
  } else {
    DEBUG("Error!");
  }
  struct netif *ap_netif = netif_list;
  while (ap_netif) {
    if (ap_netif->num == SOFTAP_IF) {
      ap_netif->input = arp_proxy;
      break;
    }
    ap_netif = ap_netif->next;
  }
#endif
  DEBUG("|   NEW SSID: ", 1); DEBUG(wifi.ext.newSSID, 1);
  if (wifi.ext.hidden) DEBUG(" (Hidden)", 1);
  DEBUG("\n|   NEW PASS: ", 1); DEBUG(wifi.ext.newPass);
  DEBUG(debugSeparator);  
  DEBUG("| Network:");
  DEBUG("|   IP: ", 1); DEBUG(WiFi.softAPIP());
#if defined(ESP32)
  DEBUG("|   Netmask: ", 1); DEBUG(WiFi.softAPSubnetMask());
#elif defined(ESP8266)
  ip_info info;
  wifi_get_ip_info(SOFTAP_IF, &info);
  DEBUG("|   Netmask: ", 1); DEBUG(IPAddress(info.netmask.addr));
#endif
  DEBUG("|   DNS: ", 1); DEBUG(toIP(wifi.ext.dns).toString());
  DEBUG(macInfo);
  return wifiMode;
}
#endif

inline uint8_t initWifi(uint8_t mo) {
  WifiEventsInit();
  DEBUG("| WiFi:");
  if (!loadWifiConfig()) {loadWifiDefaults();}
  if (mo == 1 or ( mo == 0 and wifi.mode == 1)) {wifiMode = wifiStartAP();}  
  if (mo == 2 or ( mo == 0 and wifi.mode == 2)) {wifiMode = wifiStartSTA();}  
#if(EXTENDER)
  if (mo == 3 or ( mo == 0 and wifi.mode == 3)) {wifiMode = wifiStartEXT();}
#endif
  if (wifiMode == 0 or wifi.mode == 0) {wifiMode = wifiStartAP();}
  if (wifiMode == 1) {server = new AsyncWebServer(wifi.ap.port);}
  if (wifiMode == 2) {server = new AsyncWebServer(wifi.sta.port);}
#if(EXTENDER)
  if (wifiMode == 3) {server = new AsyncWebServer(80);}
#endif
  DEBUG(debugSeparator);
  return wifiMode;
}

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  bool canHandle(__unused AsyncWebServerRequest *request) const override {
    return true;
  }
  void handleRequest(AsyncWebServerRequest *request) {
  String redir = "http://" + WiFi.softAPIP().toString();
  if (wifi.ap.captive == true and strncmp(wifi.ap.domain, "http://", 7) == 0) {redir = String(wifi.ap.domain);}
  if (wifi.ap.port != 80) {redir += ":" + String(wifi.ap.port);}
  request->redirect(redir);
  }
};









//
