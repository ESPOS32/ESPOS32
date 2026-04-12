#pragma once

#include <EEPROM.h>

class ResetCounter {
public:
  using Callback = void (*)();

  static void begin(uint8_t limit, uint16_t addr, uint16_t wait, Callback cb) {
    if(!limit) return;
    _addr = addr;
    _wait = wait;
    load();
    if (value() >= limit) {
      value() = 0;
      save();       
      cb();
      return;
    }
    value()++;
    save();
  }

  static uint8_t get() {
    return value();
  }

  static void clear() {
    if (_wait) {
      DEBUG("| Wait reset button ", 1);
      unsigned long start = millis();
      while (millis() - start < _wait * 1000) {DEBUG(".", 1); delay(250);}
      DEBUG("");
      }
    value() = 0;
    save();
  }

private:
  static void load() {
    uint8_t v = EEPROM.read(_addr);
    value() = v;
  }

  static void save() {
    EEPROM.write(_addr, value());
    EEPROM.commit();
  }

  static uint8_t& value() {
    static uint8_t v = 0;
    return v;
  }
  static inline uint16_t _wait = 0;
  static inline uint16_t _addr = 0;
};



class TarExtractor {
public:
    TarExtractor() {
        reset();
    }

    void begin(const String &tarPath) {
        reset();
        DEBUG ("UPLOAD: " + tarPath);
        enabled = tarPath.endsWith(".tar");
        if (!enabled) return;

        // teljes path → könyvtár + fájlnév
        int slash = tarPath.lastIndexOf('/');
        String tarDir = (slash >= 0) ? tarPath.substring(0, slash) : "";
        String tarFileName = (slash >= 0) ? tarPath.substring(slash + 1) : tarPath;

        // *** EZ A FONTOS ***
        // fullFS.tar mindig rootba megy, függetlenül a path-tól
        if (tarFileName == "fullFS.tar") {
            baseDir = "";
            return;
        }

        // normál eset
        baseDir = tarDir + "/" + tarFileName;
        int dot = baseDir.lastIndexOf('.');
        if (dot > 0) baseDir = baseDir.substring(0, dot);

        if (baseDir == "/") baseDir = "";

        if (!LittleFS.exists(baseDir)) {
            LittleFS.mkdir(baseDir);
        }
    }


    bool isEnabled() const { return enabled; }

    void processChunk(const uint8_t *data, size_t len) {
        if (!enabled) return;

        size_t offset = 0;

        while (offset < len) {
            size_t toCopy = min(bytesNeeded, len - offset);

            switch (state) {

            case READ_HEADER:
                memcpy(header + headerPos, data + offset, toCopy);
                headerPos += toCopy;
                bytesNeeded -= toCopy;

                if (bytesNeeded == 0) {
                    if (isEndOfArchive()) {
                        reset();
                        return;
                    }

                    parseHeader();

                    // ha könyvtár volt, parseHeader már elintézte, lépünk tovább
                    if (fileName.length() == 0 && fileSize == 0) {
                        state = READ_HEADER;
                        bytesNeeded = 512;
                        headerPos = 0;
                        break;
                    }

                    if (fileSize > 0 && fileName.length() > 0) {
                        openOutputFile();
                        state = READ_FILEDATA;
                        bytesNeeded = fileSize;
                    } else {
                        state = SKIP_PADDING;
                        bytesNeeded = paddingFor(0);
                    }

                    headerPos = 0;
                }
                break;

            case READ_FILEDATA:
                if (currentFile) currentFile.write(data + offset, toCopy);
                bytesNeeded -= toCopy;

                if (bytesNeeded == 0) {
                    currentFile.close();
                    state = SKIP_PADDING;
                    bytesNeeded = paddingFor(fileSize);
                }
                break;

            case SKIP_PADDING:
                bytesNeeded -= toCopy;
                if (bytesNeeded == 0) {
                    state = READ_HEADER;
                    bytesNeeded = 512;
                }
                break;
            }

            offset += toCopy;
        }
    }

private:
    bool enabled = false;

    enum State { READ_HEADER, READ_FILEDATA, SKIP_PADDING };
    State state;

    uint8_t header[512];
    size_t headerPos;

    size_t bytesNeeded;
    size_t fileSize;

    String baseDir;
    String fileName;
    File currentFile;

    void reset() {
        enabled = false;
        state = READ_HEADER;
        headerPos = 0;
        bytesNeeded = 512;
        fileSize = 0;
        fileName = "";
        currentFile.close();
    }

    bool isEndOfArchive() {
        for (int i = 0; i < 512; i++) {
            if (header[i] != 0) return false;
        }
        return true;
    }

    size_t paddingFor(size_t size) {
        return (512 - (size % 512)) % 512;
    }

    void parseHeader() {
        char name[101];
        memcpy(name, header, 100);
        name[100] = 0;
        fileName = String(name);

        char sizeStr[13];
        memcpy(sizeStr, header + 124, 12);
        sizeStr[12] = 0;
        fileSize = strtol(sizeStr, nullptr, 8);

        char typeflag = header[156];
        bool isDir = (typeflag == '5') || fileName.endsWith("/");

        if (isDir) {
            String full = baseDir + "/" + fileName;
            if (full.endsWith("/")) full.remove(full.length() - 1);
            if (full.length() == 0) full = "/";

            if (!LittleFS.exists(full)) {
                LittleFS.mkdir(full);
            }

            // jelöljük, hogy ez könyvtár volt, nincs filedata
            fileName = "";
            fileSize = 0;
            return;
        }

        if (fileName.endsWith("/")) fileName = "";
    }

    void openOutputFile() {
        String full = baseDir + "/" + fileName;

        if (!systemFolder && full.startsWith("/system/")) {
            DEBUG ("TAR: system folder write blocked");
            return;
        }
        if (!coreFolder && full.startsWith("/core/")) {
            DEBUG ("TAR: core folder write blocked");
            return;
        }

        int slash = full.lastIndexOf('/');
        if (slash > 0) {
            String dir = full.substring(0, slash);
            if (!LittleFS.exists(dir)) LittleFS.mkdir(dir);
        }

        currentFile = LittleFS.open(full, "w");

        full.replace("//", "/");
        DEBUG ("EXTRACT: " + full);
    }
};



class TarResponse : public AsyncAbstractResponse {
public:
    TarResponse(const String &rootFsPath)
        : rootFs(rootFsPath)
    {
        _code = 200;
        _sendContentLength = false;
        _chunked = true;
        _contentLength = 0;

        setContentType("application/x-tar");

        // Root mappa neve
        String rootName = rootFsPath;
        if (rootName.endsWith("/")) rootName.remove(rootName.length() - 1);
        rootName = rootName.substring(rootName.lastIndexOf('/') + 1);

        // ===== ROOT FILTER =====
        if ((!systemFolder && rootName == "system") ||
            (!coreFolder   && rootName == "core"))
        {
            state = DONE;
            eofBlocks = 2;
            return;
        }

        stack.push_back({rootFs, ""});

        state = SEND_HEADER;
        eofBlocks = 0;
    }

    bool _sourceValid() const override {
        return true;
    }

    size_t _fillBuffer(uint8_t *buf, size_t maxLen) override {
        size_t outLen = 0;

        while (outLen == 0) {

            if (state == DONE) {
                return 0;
            }

            if (state == SEND_HEADER) {
                if (makeNextHeader(buf)) {
                    outLen = 512;
                    state = currentIsDir ? NEXT_ENTRY : SEND_DATA;
                    return outLen;
                } else {
                    state = SEND_EOF;
                }
            }

            if (state == SEND_DATA) {
                if (sendFileData(buf, maxLen, outLen)) {
                    return outLen;
                } else {
                    state = NEXT_ENTRY;
                }
            }

            if (state == NEXT_ENTRY) {
                if (!hasMoreEntries()) {
                    state = SEND_EOF;
                } else {
                    state = SEND_HEADER;
                }
            }

            if (state == SEND_EOF) {
                if (eofBlocks < 2) {
                    memset(buf, 0, 512);
                    eofBlocks++;
                    return 512;
                }
                state = DONE;
                return 0;
            }
        }

        return outLen;
    }

private:
    enum State {
        SEND_HEADER,
        SEND_DATA,
        NEXT_ENTRY,
        SEND_EOF,
        DONE
    };

    struct Entry {
        String fsPath;
        String tarPath;
    };

    State state;
    String rootFs;
    std::vector<Entry> stack;

    File currentFile;
    size_t fileBytesSent = 0;
    bool currentIsDir = false;
    int eofBlocks = 0;

    // ---- TAR header készítés ----
    void makeTarHeader(uint8_t *block,
                       const String &name,
                       size_t size,
                       bool isDir)
    {
        memset(block, 0, 512);

        snprintf((char*)block +   0, 100, "%s", name.c_str());
        snprintf((char*)block + 100,   8, "%07o", isDir ? 0755 : 0644);
        snprintf((char*)block + 108,   8, "%07o", 0);
        snprintf((char*)block + 116,   8, "%07o", 0);
        snprintf((char*)block + 124,  12, "%011o", size);
        snprintf((char*)block + 136,  12, "%011o", (unsigned int)time(nullptr));

        memset(block + 148, ' ', 8);

        block[156] = isDir ? '5' : '0';

        memcpy(block + 257, "ustar", 5);
        block[262] = '\0';
        memcpy(block + 263, "00", 2);

        unsigned int sum = 0;
        for (int i = 0; i < 512; i++) sum += block[i];

        snprintf((char*)block + 148, 8, "%06o", sum);
        block[154] = '\0';
        block[155] = ' ';
    }

    // ---- Következő header ----
    bool makeNextHeader(uint8_t *block) {
        if (stack.empty()) return false;

        Entry e = stack.back();
        stack.pop_back();

        File f = LittleFS.open(e.fsPath, "r");
        if (!f) return false;

        currentIsDir = f.isDirectory();
        size_t size = currentIsDir ? 0 : f.size();

        String tarName = e.tarPath;
        if (currentIsDir && !tarName.endsWith("/")) tarName += "/";

        makeTarHeader(block, tarName, size, currentIsDir);

        if (currentIsDir) {
            File c;
            while ((c = f.openNextFile())) {
                String fullName = String(c.name());
                String baseName = fullName.substring(fullName.lastIndexOf('/') + 1);

                // ===== SYSTEM FOLDER FILTER =====
                if (!systemFolder && baseName == "system") {
                    c.close();
                    continue;
                }

                // ===== CORE FOLDER FILTER =====
                if (!coreFolder && baseName == "core") {
                    c.close();
                    continue;
                }

                String childFs = e.fsPath;
                if (!childFs.endsWith("/")) childFs += "/";
                childFs += baseName;

                String childTar = tarName + baseName;

                stack.push_back({childFs, childTar});
                c.close();
            }
            f.close();
            currentFile = File();
            fileBytesSent = 0;
        } else {
            currentFile = f;
            fileBytesSent = 0;
        }

        return true;
    }

    // ---- Fájl adat küldése ----
    bool sendFileData(uint8_t *buffer, size_t maxLen, size_t &outLen) {
        outLen = 0;
        if (!currentFile) return false;

        size_t toRead = maxLen;
        if (toRead > 4096) toRead = 4096;

        size_t n = currentFile.read(buffer, toRead);
        if (n > 0) {
            outLen = n;
            fileBytesSent += n;
            return true;
        }

        size_t pad = (512 - (fileBytesSent % 512)) % 512;
        if (pad) {
            memset(buffer, 0, pad);
            outLen = pad;
            fileBytesSent += pad;
            currentFile.close();
            currentFile = File();
            return true;
        }

        currentFile.close();
        currentFile = File();
        return false;
    }

    bool hasMoreEntries() {
        return !stack.empty();
    }
};
