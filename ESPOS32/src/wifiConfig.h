// =======================
//  USER WIFI MODE ENUM
//  (Does NOT conflict with ESP-IDF enums)
// =======================
enum WifiUserMode : uint8_t {
    USER_WIFI_AP  = 1,
    USER_WIFI_STA = 2,
    USER_WIFI_EXT = 3
};

// =======================
//  IP STRUCT
// =======================
struct IP4 {
    uint8_t a = 0, b = 0, c = 0, d = 0;
};

static inline IPAddress toIP(const IP4 &ip) {
    return IPAddress(ip.a, ip.b, ip.c, ip.d);
}

// =======================
//  MAC STRUCT
// =======================
struct MAC {
    char m[6][3];
};

struct WifiMACConfig {
    uint8_t macMode;   // 0 = default, 1 = random, 2 = custom
    MAC customMac;
};

// =======================
//  AP CONFIG
// =======================
struct WifiAPConfig {
    char ssid[32];
    char password[64];
    bool hidden;
    uint8_t channel;
    uint8_t power;
    bool captive;
    char domain[64];
    IP4 ip;
    uint16_t port;
    bool https;
};

// =======================
//  STA CONFIG
// =======================
struct WifiSTAConfig {
    char ssid[32];
    char password[64];
    bool dhcp;
    IP4 ip;
    IP4 gateway;
    IP4 mask;
    IP4 dns;
    uint16_t port;
    bool https;
};

// =======================
//  EXT CONFIG
// =======================
struct WifiEXTConfig {
    char ssid[32];
    char password[64];
    char newSSID[32];
    char newPass[64];
    bool hidden;
    uint8_t channel;
    
    IP4 ip;
    IP4 mask;
    IP4 dns;
    IP4 startip;
};

// =======================
//  MAIN WIFI CONFIG STRUCT
// =======================
struct WifiConfig {
    WifiUserMode mode;
    WifiAPConfig ap;
    WifiSTAConfig sta;
    WifiEXTConfig ext;
    WifiMACConfig mac;
};

extern WifiConfig wifi;

// =======================
//  DEFAULTS
// =======================
static inline void loadWifiDefaults() {

    wifi.mode = USER_WIFI_AP;

    // AP defaults
    strcpy(wifi.ap.ssid, "ESP32-AP");
    strcpy(wifi.ap.password, "12345678");
    wifi.ap.hidden = false;
    wifi.ap.channel = 17;
    wifi.ap.power = 4;
    wifi.ap.captive = true;
    strcpy(wifi.ap.domain, "http://espos.hu");
    wifi.ap.ip = {192,168,4,1};
    wifi.ap.port = 80;
    wifi.ap.https = false;

    // STA defaults
    strcpy(wifi.sta.ssid, "");
    strcpy(wifi.sta.password, "");
    wifi.sta.dhcp = false;
    wifi.sta.ip = {192,168,0,88};
    wifi.sta.gateway = {192,168,0,1};
    wifi.sta.mask = {255,255,255,0};
    wifi.sta.dns = {8,8,8,8};
    wifi.sta.port = 80;
    wifi.sta.https = false;

    // EXT defaults
    strcpy(wifi.ext.ssid, "");
    strcpy(wifi.ext.password, "");
    strcpy(wifi.ext.newSSID, "ESP32-EXT");
    strcpy(wifi.ext.newPass, "12345678");
    wifi.ext.hidden = false;
    wifi.ext.channel = 17;
    wifi.ext.ip = {192,168,50,1};
    wifi.ext.mask = {255,255,255,0};
    wifi.ext.dns = {8,8,8,8};
    wifi.ext.startip = {192,168,50,100};

    // GLOBAL MAC defaults
    wifi.mac.macMode = 0;
    for (int i = 0; i < 6; i++) { snprintf(wifi.mac.customMac.m[i], 3, "00"); }
}


// =======================
//  BINARY SAVE
// =======================
static inline bool saveWifiConfig() {
    LittleFS.mkdir("/system/setup");
    File f = LittleFS.open(SETUP_WIFI_FILE, "w");
    if (!f) return false;

    size_t written = f.write((uint8_t*)&wifi, sizeof(WifiConfig));
    f.close();

    return (written == sizeof(WifiConfig));
}

// =======================
//  BINARY LOAD
// =======================
static inline bool loadWifiConfig() {

    if (!LittleFS.exists(SETUP_WIFI_FILE))
        return false;

    File f = LittleFS.open(SETUP_WIFI_FILE, "r");
    if (!f) return false;

    if (f.size() != sizeof(WifiConfig)) {
        f.close();
        return false;
    }

    size_t read = f.read((uint8_t*)&wifi, sizeof(WifiConfig));
    f.close();

    return (read == sizeof(WifiConfig));
}
