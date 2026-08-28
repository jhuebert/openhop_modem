// =============================================================
// node_state.cpp — see header.
// =============================================================
#if defined(BOARD_HELTEC_T114)

#include "node_state.h"

#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

using namespace Adafruit_LittleFS_Namespace;

namespace NodeState {

static const char* PATH_NAME    = "/state_name";
static const char* PATH_STANDBY = "/state_stby";
static const char* PATH_AUTOCAD = "/state_acad";
static const char* PATH_FBREP   = "/state_fbrep";
static const char* PATH_RADIO   = "/state_radio";

static char  s_name[24]  = "";
static bool  s_standby   = false;
static bool  s_auto_cad  = false;
static bool  s_fbrep     = false;
static bool  s_ready     = false;

static void readFile(const char* path, char* buf, size_t cap) {
    File f(InternalFS);
    if (!f.open(path, FILE_O_READ)) { buf[0] = '\0'; return; }
    int n = f.read((uint8_t*)buf, cap - 1);
    if (n < 0) n = 0;
    buf[n] = '\0';
    f.close();
}

static void writeFile(const char* path, const void* data, size_t len) {
    InternalFS.remove(path);   // overwrite
    File f(InternalFS);
    if (!f.open(path, FILE_O_WRITE)) return;
    f.write((const uint8_t*)data, len);
    f.close();
}

void begin() {
    if (s_ready) return;
    if (!InternalFS.begin()) {
        // First time the LittleFS partition was used — format it.
        InternalFS.format();
        InternalFS.begin();
    }

    readFile(PATH_NAME, s_name, sizeof(s_name));

    char tmp[4] = {};
    readFile(PATH_STANDBY, tmp, sizeof(tmp));
    s_standby = (tmp[0] == '1');

    char tmp2[4] = {};
    readFile(PATH_AUTOCAD, tmp2, sizeof(tmp2));
    s_auto_cad = (tmp2[0] == '1');

    char tmp3[4] = {};
    readFile(PATH_FBREP, tmp3, sizeof(tmp3));
    s_fbrep = (tmp3[0] == '1');

    s_ready = true;
}

const char* getDisplayName() { return s_name; }
bool getStandby()            { return s_standby; }
bool getAutoCad()            { return s_auto_cad; }
bool getFallbackRepeat()     { return s_fbrep; }

void setDisplayName(const char* name) {
    if (!name) name = "";
    if (strncmp(s_name, name, sizeof(s_name)) == 0) return;   // unchanged
    snprintf(s_name, sizeof(s_name), "%s", name);
    writeFile(PATH_NAME, s_name, strlen(s_name));
}

void setStandby(bool on) {
    if (s_standby == on) return;
    s_standby = on;
    char c = on ? '1' : '0';
    writeFile(PATH_STANDBY, &c, 1);
}

void setAutoCad(bool on) {
    if (s_auto_cad == on) return;
    s_auto_cad = on;
    char c = on ? '1' : '0';
    writeFile(PATH_AUTOCAD, &c, 1);
}

void setFallbackRepeat(bool on) {
    if (s_fbrep == on) return;
    s_fbrep = on;
    char c = on ? '1' : '0';
    writeFile(PATH_FBREP, &c, 1);
}

bool loadRadioConfig(uint8_t* out, size_t len) {
    begin();
    File f(InternalFS);
    if (!f.open(PATH_RADIO, FILE_O_READ)) return false;
    int n = f.read(out, len);
    f.close();
    return n == (int)len;
}

void saveRadioConfig(const uint8_t* data, size_t len) {
    begin();
    writeFile(PATH_RADIO, data, len);
}

}   // namespace NodeState

#endif   // BOARD_HELTEC_T114
