#pragma once
#include "espos_core.h"
#include "fileSystemClass.h"
#include "raw_html.h"

bool rm_rf(const String &path);

extern bool passwdLoad();
extern bool passwdSave();

String editLast;


inline size_t dirSize(File dir){
    if(!dir || !dir.isDirectory()) return 0;
    size_t total = 0;
    File f = dir.openNextFile();
    while(f){
        if(f.isDirectory()){
            total += dirSize(f); // rekurzió File objektummal
        } else {
            total += f.size();
        }
        f.close();
        f = dir.openNextFile();
    }
    return total;
}

/* ===================== /list ===================== */
struct Entry { String name; bool dir; size_t size; };

inline void handleList(AsyncWebServerRequest *r) {
    String path = "/";
    if (r->hasParam("path", true)) path = r->getParam("path", true)->value();

    File dir = LittleFS.open(path, "r");
    
    if (!dir || !dir.isDirectory()) {
        r->send(200, "application/json", "[]");
        return;
    }

    std::vector<Entry> v;
    File x = dir.openNextFile();
    while (x) {
        String fullName = String(x.name());
        String n = fullName.substring(fullName.lastIndexOf('/') + 1);

        if (n == "..") { x = dir.openNextFile(); continue; }

        if (path == "/") {
            if (!coreFolder && n == "core") { x = dir.openNextFile(); continue; }
            if (!systemFolder && n == "system") { x = dir.openNextFile(); continue; }
        }

        size_t sz = x.isDirectory() ? dirSize(x) : x.size();
        v.push_back({n, x.isDirectory(), sz});
        x = dir.openNextFile();
    }

    std::sort(v.begin(), v.end(), [](const Entry &a, const Entry &b) {
        if (a.name.equalsIgnoreCase("core")) return true;
        if (b.name.equalsIgnoreCase("core")) return false;
        if (a.name.equalsIgnoreCase("system")) return true;
        if (b.name.equalsIgnoreCase("system")) return false;
        if (a.dir != b.dir) return a.dir;
        String A = a.name; A.toLowerCase();
        String B = b.name; B.toLowerCase();
        return A < B;
    });

    String j = "[";
    if (editLast != "") j += "{\"default\":\"" + editLast + "\"},";
    editLast = "";
    for (size_t i = 0; i < v.size(); i++) {
        if (i) j += ",";
        j += "{\"name\":\"" + v[i].name +
             "\",\"dir\":" + (v[i].dir ? "true" : "false") +
             ",\"size\":" + String(v[i].size) + "}";
    }
    j += "]";
    r->send(200, "application/json", j);
}

/* ===================== /list_s ===================== */
inline void handleListSafe(AsyncWebServerRequest *r) {
    String path = "/";
    if (r->hasParam("path", true))
        path = r->getParam("path", true)->value();

    File dir = LittleFS.open(path, "r");
    if (!dir || !dir.isDirectory()) {
        r->send(200, "application/json", "[]");
        return;
    }

    std::vector<Entry> v;
    while (true) {
        File f = dir.openNextFile();
        if (!f) break;

        const char *raw = f.name();
        if (!raw) { f.close(); continue; }

        bool valid = true;
        for (int i = 0; i < 128; i++) {
            char c = raw[i];
            if (c == 0) break;
            if (c < 32 || c > 126) { valid = false; break; }
        }
        if (!valid) { f.close(); continue; }

        String full = String(raw);
        int idx = full.lastIndexOf('/');
        if (idx < 0) idx = -1;
        String name = full.substring(idx + 1);
        if (name.length() == 0) { f.close(); continue; }

        bool isDir = f.isDirectory();
        size_t sz = isDir ? 0 : f.size();

        v.push_back({name, isDir, sz});
        f.close();
    }

    std::sort(v.begin(), v.end(), [](const Entry &a, const Entry &b) {
        if (a.dir != b.dir) return a.dir;
        String A = a.name; A.toLowerCase();
        String B = b.name; B.toLowerCase();
        return A < B;
    });

    String j = "[";
    for (size_t i = 0; i < v.size(); i++) {
        if (i) j += ",";
        j += "{\"name\":\"" + v[i].name +
             "\",\"dir\":" + (v[i].dir ? "true" : "false") +
             ",\"size\":" + String(v[i].size) + "}";
    }
    j += "]";
    r->send(200, "application/json", j);
}

/* ===================== /flashfree ===================== */
inline void handleFlashFree(AsyncWebServerRequest *r) {
#if defined(ESP32)
    size_t total = LittleFS.totalBytes();
    size_t used  = LittleFS.usedBytes();
#elif defined(ESP8266)
    FSInfo info;
    LittleFS.info(info);
    size_t total = info.totalBytes;
    size_t used  = info.usedBytes;
#endif
    r->send(200, "application/json",
        "{\"total\":" + String(total / 1024) +
        ",\"used\":"  + String(used  / 1024) + "}");
}


/* ===================== /upload ===================== */
inline void handleUpload(AsyncWebServerRequest *r, uint8_t *d, size_t l, size_t i, size_t t) {

    if (i == 0) {
        String name = r->getParam("name")->value();
        String path = r->getParam("path")->value();
        if (!path.endsWith("/")) path += "/";

        TarExtractor *tar = new TarExtractor();
        tar->begin(path + name);
        r->_tempObject = tar;

        if (!name.endsWith(".tar")) {
            r->_tempFile = LittleFS.open(path + name, "w");
        }
    }

    TarExtractor *tar = (TarExtractor*) r->_tempObject;

    if (tar && tar->isEnabled()) {
        tar->processChunk(d, l);
    } else {
        if (r->_tempFile) r->_tempFile.write(d, l);
    }

    if (i + l == t) {
        if (tar && tar->isEnabled()) {
            delete tar;
        } else {
            if (r->_tempFile) r->_tempFile.close();
        }
    }
}

/* ===================== /download ===================== */
inline void handleDownload(AsyncWebServerRequest *r) {

    if (!r->hasParam("path", true)) {
        r->send(400, "text/plain", "Missing path");
        return;
    }

    String path = r->getParam("path", true)->value();

    if (path == "/") {
        TarResponse *response = new TarResponse("/");
        response->addHeader("Content-Disposition",
                            "attachment; filename=\"fullFS.tar\"");
        r->send(response);
        return;
    }

    if (!LittleFS.exists(path)) {
        r->send(404, "text/plain", "Not found");
        return;
    }

    File f = LittleFS.open(path, "r");
    bool isDir = f.isDirectory();
    f.close();

    if (!isDir) {
        r->send(LittleFS, path, "application/octet-stream", true);
        return;
    }

    String base = path.substring(path.lastIndexOf('/') + 1);
    if (base == "") base = "root";

    TarResponse *response = new TarResponse(path);
    response->addHeader("Content-Disposition",
                        "attachment; filename=\"" + base + ".tar\"");
    r->send(response);
}

/* ===================== /format ===================== */

inline void afterFormat(bool keep) {
    LittleFS.mkdir("/core");
    LittleFS.mkdir("/core/admin");
    LittleFS.mkdir("/core/setup");
    LittleFS.mkdir("/system");
    LittleFS.mkdir("/system/setup");
    if (!keep) {
      loadWifiDefaults();
      saveWifiConfig();
      }
    File f = LittleFS.open(DEFPAGE, "w");
    for (size_t i = 0; i < strlen_P(index_html); i++) {
      char c = pgm_read_byte_near(index_html + i);
      f.write(c);
    }
    f.close();
}

inline void handleFormat(AsyncWebServerRequest *r) {
/*
    if (!r->hasParam("confirm", true) ||
        r->getParam("confirm", true)->value() != "YES") {
        r->send(400, "application/json",
                "{\"ok\":false,\"error\":\"CONFIRM_REQUIRED\"}");
        return;
    }
*/
    // Format
    if (!LittleFS.format()) {
        r->send(500, "application/json", "{\"ok\":false,\"error\":\"FORMAT_FAILED\"}");
        return;
    }
#if defined(ESP32)
    if (!LittleFS.begin(true)) {
        r->send(500, "application/json", "{\"ok\":false,\"error\":\"MOUNT_FAILED_AFTER_FORMAT\"}");
        return;
    }
#elif defined(ESP8266)
    if (!LittleFS.begin()) {
        r->send(500, "application/json", "{\"ok\":false,\"error\":\"MOUNT_FAILED_AFTER_FORMAT\"}");
        return;
    }
#endif
    afterFormat(true);
    r->send(200, "application/json",
            "{\"ok\":true,\"msg\":\"FS formatted and core/system created\"}");
}

inline void handleDefault(AsyncWebServerRequest *r) {
    loadWifiDefaults();
    saveWifiConfig();
    passwdLoad();
    r->send(200, "text/html", init_setup_html, wifiProcessor);
}

inline void handleSaveAll(AsyncWebServerRequest *r) {
    saveWifiConfig();
    passwdSave();
    r->send(200, "text/html", init_setup_html, wifiProcessor);
}


/* ===================== /mkdir ===================== */
inline void handleMkdir(AsyncWebServerRequest *r) {
    r->send(LittleFS.mkdir(r->getParam("path", true)->value()) ? 200 : 500);
}

/* ===================== /delete ===================== */
inline void handleDelete(AsyncWebServerRequest *r) {
    r->send(rm_rf(r->getParam("path", true)->value()) ? 200 : 500);
}

/* ===================== /rename ===================== */
inline void handleRename(AsyncWebServerRequest *r) {
    r->send(LittleFS.rename(
        r->getParam("old", true)->value(),
        r->getParam("new", true)->value()
    ) ? 200 : 500);
}

/* ===================== /editor.html ===================== */
inline void handleEditor(AsyncWebServerRequest *r) {
    AsyncWebServerResponse* response;
    if (!(LittleFS.exists(EDITHTML))) {
      response = r->beginResponse(200, "text/html", edit_html);
      } else {
      response = r->beginResponse(LittleFS, EDITHTML, "text/html");
      }   
    String path = "";
    if (r->hasParam("path", true)) path = r->getParam("path", true)->value();
    response->addHeader("Set-Cookie", "editpath=" + path);
    r->send(response);
    editLast = path;
}

/* ===================== /style.js ===================== */
inline String styleProcessor(const String& var) {
    if (var.equals(F("STYLEFILE")))  return STYLE_FILE;
    return String();
}
inline void handleStyle(AsyncWebServerRequest *r) {
    r->send(200, "text/html", style_html, styleProcessor);
}








/* ================= REKURZÍV TÖRLÉS ================= */
inline bool rm_rf(const String &path){
    File dir = LittleFS.open(path, "r");
    if(!dir) return false;

    if(!dir.isDirectory()){
        dir.close();
        return LittleFS.remove(path);
    }

    std::vector<String> items;
    File f = dir.openNextFile();
    while(f){
        String name = String(f.name());
        if(!name.startsWith("/")) name = path + "/" + name;
        items.push_back(name);
        f.close();
        f = dir.openNextFile();
    }
    dir.close();

    for(const String &p : items){
        File x = LittleFS.open(p, "r");
        if(x && x.isDirectory()){
            x.close();
            rm_rf(p);
        } else {
            if(x) x.close();
            LittleFS.remove(p);
        }
    }
    return LittleFS.rmdir(path);
}

inline String fmtSize() {
#if defined(ESP32)
    size_t total = LittleFS.totalBytes();
    size_t used  = LittleFS.usedBytes();
#elif defined(ESP8266)
    FSInfo info;
    LittleFS.info(info);
    size_t total = info.totalBytes;
    size_t used  = info.usedBytes;
#endif
    return "|   Total: " + String(total / 1024) + "kB\n|   Free : " + String((total - used) / 1024) + "kB";
}

inline void onFormatRequired() {
    DEBUG("|   !!! Force format !!!");
    if (LittleFS.format()) {
      afterFormat(false);
      DEBUG("|   Format OK, restarting...");
      } else {
      DEBUG("|   !!! Format FAILED !!!");
      }
    DEBUG(debugSeparator);
    delay(5000);
    ESP.restart();
    return;
}

inline void initFS(bool f, uint8_t rst, uint16_t wait) {
    DEBUG ("| LittleFS:");
    ResetCounter::begin(rst, 511, wait, onFormatRequired); 
    if (rst) DEBUG("|   Format DownCount: ", 1);
    if (rst) DEBUG(rst - ResetCounter::get() + 1);
    if (LittleFS.begin() and !f) {
      DEBUG (fmtSize());
      loadWifiConfig();
      passwdLoad();
    } else {
      DEBUG ("|   \nLittleFS mount failed!");
      onFormatRequired();
      }
    if (!(LittleFS.exists(SETUP_WIFI_FILE))) DEBUG("|   Not found wifi.bin!!!");
    DEBUG(debugSeparator);
}




//
