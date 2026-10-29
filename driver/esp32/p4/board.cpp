#include "../backend_select.h"
#if defined(ARDUINO_ARCH_ESP32) && PAL_ESP32_BACKEND_P4
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <SD_MMC.h>
#include <driver/i2s.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <esp_heap_caps.h>
#include <esp_rom_crc.h>
#include <sys/stat.h>
#include <atomic>
#include <math.h>
#include "board.h"
#include "controls.h"
#include "power_state.h"
#include "codec/es8311.h"
extern "C" {
#include "../../../src/driver.h"
#include "../../../src/audio.h"
#include "../../../src/input.h"
#include "../../../src/main.h"
#include "../../../src/play.h"
#include "../../../src/ui.h"
#include "../../../src/uigame.h"
#include "../../../src/util.h"
}

// Pin map follows Waveshare's LCD35 board layer. Unmirrored USB-right landscape.
// Transactions must be enabled: this GFX backend applies the requested 80 MHz
// and SPI mode 3 in beginTransaction(), not in begin(). Disabling them leaves
// Arduino's default 1 MHz clock and takes about one second per game frame.
static Arduino_HWSPI lcdBus(26, 23, 21, 20, GFX_NOT_DEFINED, &SPI, true);
static Arduino_ST7796 lcd(&lcdBus, 27, p4::Rotation, true, 320, 480);
static uint16_t palette[256];
static uint16_t scanlines[p4::ScreenW * 16];
static uint16_t *scenePixels;
// Explicit PSRAM allocation keeps the large transparent overlay off internal RAM.
class OverlayCanvas : public Arduino_Canvas {
public:
    OverlayCanvas() : Arduino_Canvas(p4::ScreenW, p4::ScreenH, nullptr) {}
    bool begin(int32_t = GFX_NOT_DEFINED) override {
        _framebuffer = (uint16_t *)heap_caps_calloc(p4::ScreenW*p4::ScreenH,
            sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        return _framebuffer != nullptr;
    }
};
static OverlayCanvas overlay;
static p4::Controls controls;
static p4::KeyRepeater keyRepeater;
static bool displayReady;
static std::atomic<bool> audioReady{false};
static std::atomic<bool> gameReady{false};
static SemaphoreHandle_t audioMutex;
static TaskHandle_t audioTask;
static SemaphoreHandle_t audioParked;
static std::atomic<bool> audioParkRequested{false};
static p4::PmicPowerButton powerButton;
static bool powerKeyReady, powerLogReady;
static uint32_t lastKeyPoll;
static p4::GameClock gameClock;
extern "C" uint32_t P4_GameMilliseconds(uint32_t raw) { return gameClock.milliseconds(raw); }
static es8311_handle_t codec;
static uint32_t lastTouchPoll, lastTouchSuccess;
static int volumeLevel = p4::DefaultVolume;
static char powerLabel[80] = "POWER: unavailable";
static uint32_t lastPowerPoll;

static bool readPowerRegister(uint8_t reg, uint8_t &value) {
    Wire.beginTransmission(0x34);
    Wire.write(reg);
    if (Wire.endTransmission(false)!=0 || Wire.requestFrom(0x34,1)!=1) return false;
    value=Wire.read();
    return true;
}

static bool writePowerRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(0x34);
    Wire.write(reg); Wire.write(value);
    return Wire.endTransmission()==0;
}

static bool readPowerKey(uint8_t &irq) {
    if (!readPowerRegister(0x49,irq)) return false;
    irq &= 0x0c;
    // Clear only key events; preserve power, battery and protection flags.
    return !irq || writePowerRegister(0x49,irq);
}

static void recordPower(const char *event) {
    uint8_t s0=0xff,s1=0xff,irq0=0xff,irq1=0xff,irq2=0xff,percent=0xff;
    readPowerRegister(0x00,s0); readPowerRegister(0x01,s1);
    readPowerRegister(0x48,irq0); readPowerRegister(0x49,irq1);
    readPowerRegister(0x4a,irq2); readPowerRegister(0xa4,percent);
    uint8_t hi=0,lo=0;
    int batteryMv=readPowerRegister(0x34,hi) && readPowerRegister(0x35,lo) ?
        ((hi&0x3f)<<8)|lo : -1;
    char line[240];
    snprintf(line,sizeof(line),"%lu %s reset=%d gpio49=%d status=%02x/%02x irq=%02x/%02x/%02x battery=%u vbat_mv=%d\n",
        (unsigned long)millis(),event,(int)esp_reset_reason(),digitalRead(49),
        s0,s1,irq0,irq1,irq2,percent,batteryMv);
    Serial.print(line);
    // Bounded diagnostic file, retained when USB is absent. Never overwrite
    // game data or truncate an existing log; stop appending at 128 KiB.
    if (!powerLogReady) return;
    File log=SD_MMC.open("/p4-power.log",FILE_APPEND);
    if (log && log.size()<128*1024) log.print(line);
    log.close();
}

static void initPowerKey() {
    uint8_t enabled=0,irq=0;
    // Keep all existing IRQ enables and the hardware six-second shutdown.
    powerKeyReady=readPowerRegister(0x41,enabled) &&
        writePowerRegister(0x41,enabled|0x08) && readPowerKey(irq);
    powerButton.arm(millis());
    Serial.printf("P4 PWR: PMIC short-press events %s; GPIO49 diagnostic only\n",
                  powerKeyReady?"ready":"unavailable");
}

static bool refreshPower(bool force, uint32_t now) {
    if (!force && (!controls.state.toolbar || now-lastPowerPoll<2000)) return false;
    lastPowerPoll=now;
    uint8_t status1=0, status2=0, percent=255;
    char label[sizeof(powerLabel)];
    // AXP2101 read-only telemetry: REG00 VBUS_GOOD[5]/BAT_PRESENT[3],
    // REG01 charge direction[6:5], REGA4 fuel-gauge percentage (0..100).
    if (!readPowerRegister(0x00,status1) || !readPowerRegister(0x01,status2))
        snprintf(label,sizeof(label),"POWER: unavailable");
    else {
        const bool battery=(status1&0x08)!=0, usb=(status1&0x20)!=0;
        const char *source=usb ? "USB power" : (battery ? "Battery power" : "Power unknown");
        if (!battery) snprintf(label,sizeof(label),"No battery | %s",source);
        else {
            bool valid=readPowerRegister(0xa4,percent) && percent<=100;
            char level[8];
            if (valid) snprintf(level,sizeof(level),"%u%%",percent);
            else snprintf(level,sizeof(level),"--%%");
            const char *charge=((status2>>5)&3)==1 ? " | Charging" :
                ((status2&7)==4 ? " | Full" : "");
            snprintf(label,sizeof(label),"BAT %s | %s%s",level,source,charge);
        }
    }
    if (strcmp(label,powerLabel)==0) return false;
    snprintf(powerLabel,sizeof(powerLabel),"%s",label);
    Serial.printf("P4 power: %s\n",powerLabel);
    return true;
}

static esp_err_t applyVolumeLevel(int level) {
    esp_err_t error = es8311_voice_gain_set(codec, p4::volumeGainHalfDb(level));
    if (error == ESP_OK) error = es8311_voice_mute(codec, level <= 0);
    return error;
}

static void presentRect(p4::Rect r, bool profile = false) {
    if (!displayReady || !scenePixels || !overlay.getFramebuffer()) return;
    int right = r.x+r.w, bottom = r.y+r.h;
    if (r.x<0) r.x=0;
    if (r.y<0) r.y=0;
    if (right>p4::ScreenW) right=p4::ScreenW;
    if (bottom>p4::ScreenH) bottom=p4::ScreenH;
    r.w=right-r.x;
    if (r.w<=0 || bottom<=r.y) return;
    p4::Rect areas[3]{};
    int areaCount=0;
    const auto &state=controls.state;
    if (state.toolbar) areas[areaCount++]=p4::Toolbar;
    if (state.joystick) areas[areaCount++]=p4::CenterMarkerBounds;
    uint32_t direction=p4::visibleDirection(state);
    if (direction) areas[areaCount++]=p4::directionBounds(direction);
    uint32_t composeUs=0, spiUs=0;
    for (int row=r.y;row<bottom;row+=16) {
        int count=bottom-row<16?bottom-row:16;
        uint32_t started=profile?micros():0;
        p4::composeBand(scanlines,scenePixels,overlay.getFramebuffer(),{r.x,row,r.w,count},areas,areaCount);
        uint32_t composed=profile?micros():0;
        lcd.draw16bitBeRGBBitmap(r.x,row,scanlines,r.w,count);
        if (profile) { composeUs+=composed-started; spiUs+=micros()-composed; }
    }
    if (profile) Serial.printf("P4 display: compose=%lu us SPI=%lu us\n",
                              (unsigned long)composeUs,(unsigned long)spiUs);
}

static void drawOverlay() {
    if (!overlay.getFramebuffer()) return;
    const auto &state=controls.state;
    overlay.fillScreen(0); // Transparent; never stored in the clean game scene.
    if (state.joystick)
        overlay.drawCircle(p4::JoyCenterX,p4::JoyCenterY,p4::JoyDeadzone,0xffff);
    uint32_t direction=p4::visibleDirection(state);
    if (direction) {
        int x=p4::JoyCenterX, y=p4::JoyCenterY;
        int ax=0,ay=0,bx=0,by=0,cx=0,cy=0;
        if (direction==kKeyUp) { ax=x;ay=y-47;bx=x-6;by=y-35;cx=x+6;cy=y-35; }
        else if (direction==kKeyDown) { ax=x;ay=y+47;bx=x-6;by=y+35;cx=x+6;cy=y+35; }
        else if (direction==kKeyLeft) { ax=x-47;ay=y;bx=x-35;by=y-6;cx=x-35;cy=y+6; }
        else { ax=x+47;ay=y;bx=x+35;by=y-6;cx=x+35;cy=y+6; }
        overlay.fillTriangle(ax,ay,bx,by,cx,cy,0x04d7);
        overlay.drawTriangle(ax,ay,bx,by,cx,cy,0xffff);
    }
    if (state.toolbar) {
        overlay.fillRoundRect(0,p4::Toolbar.y,p4::ScreenW,p4::Toolbar.h,8,0x1083);
        overlay.setTextColor(0xffff);
        overlay.setTextSize(1);
        overlay.setCursor(12,202); overlay.print(powerLabel);
        overlay.setTextSize(2);
        overlay.setCursor(12,232); overlay.print("VOL");
        int knob=p4::SliderLeft+(p4::SliderRight-p4::SliderLeft)*volumeLevel/p4::VolumeMax;
        overlay.fillRoundRect(p4::SliderLeft,p4::SliderY-3,
                              p4::SliderRight-p4::SliderLeft+1,6,3,0x6472);
        if (knob>p4::SliderLeft)
            overlay.fillRoundRect(p4::SliderLeft,p4::SliderY-3,knob-p4::SliderLeft+1,6,3,0x04d7);
        overlay.fillCircle(knob,p4::SliderY,8,0xffff);
        overlay.setTextSize(1);
        for (int level=0;level<=p4::VolumeMax;++level) {
            int x=p4::SliderLeft+(p4::SliderRight-p4::SliderLeft)*level/p4::VolumeMax;
            overlay.fillCircle(x,p4::SliderY,2,0xffff);
            overlay.setCursor(x-2,254); overlay.printf("%d",level);
        }
        overlay.setTextSize(2);
        overlay.setCursor(382,232); overlay.printf("%d/%d",volumeLevel,p4::VolumeMax);
        const char *labels[]={"PG UP","PG DN","STATUS","AUTO","CLOSE"};
        const uint32_t keys[]={kKeyPgUp,kKeyPgDn,kKeyStatus,kKeyAuto,0};
        overlay.setTextSize(1);
        for (int i=0;i<5;++i) {
            auto r=p4::Tools[i];
            overlay.fillRoundRect(r.x,r.y,r.w,r.h,6,(state.keys&keys[i])?0x04d7:0x2127);
            overlay.drawRoundRect(r.x,r.y,r.w,r.h,6,0x6472);
            overlay.setCursor(r.x+(r.w-strlen(labels[i])*6)/2,r.y+17);
            overlay.print(labels[i]);
        }
    }
}

static void repaintControls(const p4::ControlState &old) {
    const auto &s=controls.state;
    uint32_t oldDirection=p4::visibleDirection(old), direction=p4::visibleDirection(s);
    bool directionChanged=oldDirection!=direction;
    bool centerChanged=old.joystick!=s.joystick;
    bool panelChanged=old.toolbar!=s.toolbar || old.volume!=s.volume || old.keys!=s.keys;
    if (!directionChanged && !centerChanged && !panelChanged) return;
    drawOverlay();
    if (centerChanged) presentRect(p4::CenterMarkerBounds);
    if (directionChanged) {
        if (oldDirection) presentRect(p4::directionBounds(oldDirection));
        if (direction) presentRect(p4::directionBounds(direction));
    }
    if (panelChanged && (old.toolbar || s.toolbar)) presentRect(p4::Toolbar);
    if (old.toolbar!=s.toolbar)
        Serial.printf("P4 controls: toolbar %s\n",s.toolbar?"open":"closed");
}

static void applyRequestedVolume() {
    int value=controls.state.volume;
    if (value==volumeLevel) return;
    if (!audioReady || !codec || applyVolumeLevel(value)!=ESP_OK) {
        controls.state.volume=volumeLevel;
        return;
    }
    volumeLevel=value;
    Serial.printf("VOLUME %d/5 gain=%.1f dB%s\n",volumeLevel,
        p4::volumeGainHalfDb(volumeLevel)*0.5f,volumeLevel==0?" MUTE":"");
}

static void status(const char *message) {
    Serial.println(message);
    if (!displayReady) return;
    lcd.fillScreen(0);
    lcd.setTextSize(2);
    lcd.setTextColor(0xffff);
    lcd.setCursor(8,52);
    lcd.println(message);
    digitalWrite(28,HIGH);
    drawOverlay();
}

static bool mountCard() {
    if (!SD_MMC.setPins(43, 44, 39, 40, 41, 42)) return false;
    // Slot 0's IO supply is powered by the P4's internal LDO4 (official BSP).
    if (!SD_MMC.setPowerChannel(4)) return false;
    // Never format the user's card on a mount error. More file handles are
    // needed than the Arduino default: the engine keeps 15 MKFs open.
    return SD_MMC.begin("/sdcard", false, false, SDMMC_FREQ_DEFAULT, 32);
}

static bool dataReady() {
    const char *required[] = {"abc.mkf", "ball.mkf", "data.mkf", "f.mkf",
        "fbp.mkf", "fire.mkf", "gop.mkf", "map.mkf", "mgo.mkf", "mus.mkf",
        "pat.mkf", "rgm.mkf", "rng.mkf", "sounds.mkf", "sss.mkf",
        "word.bin", "msg.bin", "unicode_font.bin", "unicode_font.dat"};
    for (const char *name : required) {
        char path[96];
        snprintf(path, sizeof(path), "/sdcard/%s", name);
        struct stat st;
        if (stat(path, &st) || st.st_size == 0) {
            Serial.printf("Missing game data: %s\n", name);
            return false;
        }
    }
    return true;
}

// UART SD deployment mode. Only explicitly named data files are written;
// uploads use a temporary file, CRC32, then rename after verification.
static bool safeName(const char *name) {
    if (!*name || strlen(name) > 80 || strstr(name, "..") || *name == '/') return false;
    for (const char *p = name; *p; ++p)
        if (!(isalnum((unsigned char)*p) || *p == '_' || *p == '.' || *p == '/')) return false;
    if (strchr(name, '/') && (strncmp(name, "mus/", 4) || strchr(name + 4, '/'))) return false;
    const char *ext = strrchr(name, '.');
    return ext && (!strcmp(ext, ".mkf") || !strcmp(ext, ".bin") ||
                   !strcmp(ext, ".dat") || !strcmp(ext, ".pcm"));
}

static void serveCard() {
    status("SD transfer mode\nKeep USB connected");
    Serial.setTimeout(10000);
    Serial.println("PALSD READY");
    static uint8_t block[4096];
    for (;;) {
        if (!Serial.available()) { delay(1); continue; }
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line == "START") { Serial.println("RESTART"); Serial.flush(); ESP.restart(); }
        if (line == "PING") { Serial.println("PALSD READY"); continue; }
        if (line == "LIST") {
            File root = SD_MMC.open("/");
            for (File entry = root.openNextFile(); entry; entry = root.openNextFile()) {
                Serial.printf("FILE %s %llu %s\n", entry.name(),
                    (unsigned long long)entry.size(), entry.isDirectory() ? "DIR" : "DATA");
                entry.close();
            }
            root.close();
            for (const char *dirName : {"mus", "mus_pcm"}) {
                String path = String("/") + dirName;
                File dir = SD_MMC.open(path);
                if (!dir || !dir.isDirectory()) continue;
                for (File entry = dir.openNextFile(); entry; entry = dir.openNextFile()) {
                    Serial.printf("FILE %s/%s %llu %s\n", dirName, entry.name(),
                        (unsigned long long)entry.size(), entry.isDirectory() ? "DIR" : "DATA");
                    entry.close();
                }
                dir.close();
            }
            Serial.println("LIST END");
            continue;
        }
        if (line.startsWith("STAT ")) {
            const char *name = line.c_str() + 5;
            if (!safeName(name)) { Serial.println("ERR NAME"); continue; }
            char path[112]; snprintf(path, sizeof(path), "/sdcard/%s", name);
            FILE *file = fopen(path, "rb");
            if (!file) { Serial.println("STAT 0 0"); continue; }
            uint32_t crc = 0, size = 0;
            size_t n;
            while ((n = fread(block, 1, sizeof(block), file))) {
                crc = esp_rom_crc32_le(crc, block, n); size += n;
            }
            fclose(file);
            Serial.printf("STAT %lu %lu\n", (unsigned long)size, (unsigned long)crc);
            continue;
        }
        unsigned long size, expected;
        char name[81];
        if (sscanf(line.c_str(), "PUT %lu %lu %80s", &size, &expected, name) != 3 ||
            !safeName(name) || size > 256UL * 1024 * 1024) {
            Serial.println("ERR COMMAND"); continue;
        }
        char path[112], temporary[120];
        snprintf(path, sizeof(path), "/sdcard/%s", name);
        snprintf(temporary, sizeof(temporary), "%s.part", path);
        if (!strncmp(name, "mus/", 4)) mkdir("/sdcard/mus", 0755);
        FILE *file = fopen(temporary, "wb");
        if (!file) { Serial.println("ERR OPEN"); continue; }
        Serial.println("SEND");
        uint32_t crc = 0, remaining = size;
        bool ok = true;
        while (remaining) {
            size_t n = remaining < sizeof(block) ? remaining : sizeof(block);
            if (Serial.readBytes(block, n) != n || fwrite(block, 1, n, file) != n) {
                ok = false; break;
            }
            crc = esp_rom_crc32_le(crc, block, n);
            remaining -= n;
            Serial.println("ACK");
        }
        if (fclose(file)) ok = false;
        if (crc != expected) ok = false;
        if (ok && rename(temporary, path)) ok = false;
        if (!ok) remove(temporary);
        Serial.println(ok ? "OK" : "ERR TRANSFER");
    }
}

static void audioWorker(void *) {
    int16_t buffer[PAL_AUDIO_SAMPLES_PER_CHUNK * PAL_AUDIO_CHANNEL_COUNT];
    for (;;) {
        // Acknowledge only outside both the mixer mutex and i2s_write. The game
        // task can then stop I2S without suspending a task holding its locks.
        if (audioParkRequested.load(std::memory_order_acquire)) {
            xSemaphoreGive(audioParked);
            while (audioParkRequested.load(std::memory_order_acquire))
                ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        }
        memset(buffer, 0, sizeof(buffer));
        if (gameReady) {
            xSemaphoreTakeRecursive(audioMutex, portMAX_DELAY);
            AUDIO_FillBuffer(buffer, sizeof(buffer));
            xSemaphoreGiveRecursive(audioMutex);
        }
        size_t written;
        if (audioReady) i2s_write(I2S_NUM_0, buffer, sizeof(buffer), &written, portMAX_DELAY);
        else vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static esp_err_t configureCodec() {
    es8311_clock_config_t clock = {};
    clock.mclk_from_mclk_pin = true;
    clock.mclk_frequency = PAL_AUDIO_SAMPLING_RATE * 256;
    clock.sample_frequency = PAL_AUDIO_SAMPLING_RATE;
    esp_err_t result=es8311_init(codec,&clock,ES8311_RESOLUTION_16,ES8311_RESOLUTION_16);
    if (result==ESP_OK) result=es8311_sample_frequency_config(codec,clock.mclk_frequency,clock.sample_frequency);
    if (result==ESP_OK) result=applyVolumeLevel(volumeLevel);
    return result;
}

static bool startAudio() {
    pinMode(53, OUTPUT); digitalWrite(53, LOW);
    i2s_config_t config = {};
    config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    config.sample_rate = PAL_AUDIO_SAMPLING_RATE;
    config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    config.dma_buf_count = 8;
    config.dma_buf_len = PAL_AUDIO_SAMPLES_PER_CHUNK;
    config.tx_desc_auto_clear = true;
    config.mclk_multiple = I2S_MCLK_MULTIPLE_256;
    i2s_pin_config_t pins = {};
    pins.mck_io_num = 13; pins.bck_io_num = 12; pins.ws_io_num = 10;
    pins.data_out_num = 9; pins.data_in_num = -1;
    if (i2s_driver_install(I2S_NUM_0, &config, 0, nullptr) != ESP_OK) return false;
    if (i2s_set_pin(I2S_NUM_0, &pins) != ESP_OK) return false;
    codec = es8311_create(0, ES8311_ADDRESS_0);
    if (!codec) return false;
    if (configureCodec()!=ESP_OK) return false;
    audioMutex = xSemaphoreCreateRecursiveMutex();
    if (!audioMutex) return false;
    audioParked=xSemaphoreCreateBinary();
    if (!audioParked) return false;
    if (xTaskCreatePinnedToCore(audioWorker, "pal-audio", 12288, nullptr, 4, &audioTask, 0) != pdPASS)
        return false;
    digitalWrite(53, HIGH);
    return true;
}

extern "C" int DRIVER_Init(void) {
    audioReady = startAudio();
    Serial.printf("P4 audio: %s volume=%d/5 gain=%.1f dB\n", audioReady ? "ready" : "unavailable",
                  volumeLevel,p4::volumeGainHalfDb(volumeLevel)*0.5f);
    return displayReady ? 0 : -1;
}
extern "C" void DRIVER_DeInit(void) {
    gameReady = false;
    if (audioTask) { vTaskDelete(audioTask); audioTask = nullptr; }
    digitalWrite(53, LOW);
    if (audioReady) i2s_driver_uninstall(I2S_NUM_0);
    audioReady = false;
}
extern "C" void DRIVER_Audio_Lock(void) {
    if (audioMutex) xSemaphoreTakeRecursive(audioMutex, portMAX_DELAY);
}
extern "C" void DRIVER_Audio_Unlock(void) {
    if (audioMutex) xSemaphoreGiveRecursive(audioMutex);
}
extern "C" void DRIVER_UpdatePalette(const uint8_t *rgb) {
    for (int i = 0; i < 256; ++i)
    {
        uint16_t pixel = ((rgb[i*3] & 0xf8) << 8) | ((rgb[i*3+1] & 0xfc) << 3) | (rgb[i*3+2] >> 3);
        palette[i] = __builtin_bswap16(pixel); // Pre-swap once for bulk SPI bytes.
    }
}
extern "C" void DRIVER_FrameShow(uint8_t *frame, uint16_t x, uint16_t y,
                                uint16_t w, uint16_t h, uint8_t padding) {
    if (!displayReady || !scenePixels) return;
    uint32_t started=micros();
    p4::Rect changes[p4::MaxSceneChanges];
    int count=p4::updateSceneChanges(scenePixels,frame,palette,x,y,w,h,padding!=0,
                                    changes,p4::MaxSceneChanges);
    unsigned pixels=0;
    for (int i=0;i<count;++i) {
        pixels+=changes[i].w*changes[i].h;
        presentRect(changes[i]);
    }
    static uint32_t reportAt,frames,totalUs,totalPixels,maxUs;
    uint32_t elapsed=micros()-started;
    ++frames; totalUs+=elapsed; totalPixels+=pixels;
    if (elapsed>maxUs) maxUs=elapsed;
    if (millis()-reportAt>=2000) {
        Serial.printf("P4 render: avg=%lu us max=%lu us pixels=%lu/%d frames=%lu\n",
            (unsigned long)(totalUs/frames),(unsigned long)maxUs,
            (unsigned long)(totalPixels/frames),p4::ScreenW*(p4::GameBottom-p4::GameTop),
            (unsigned long)frames);
        frames=totalUs=totalPixels=maxUs=0; reportAt=millis();
    }
}

// Pause is entered from the game task, between engine operations. No PMIC
// output is switched off: Flash, PSRAM, SD and peripheral settings are retained.
static void sleepGame(uint64_t timerWakeUs=0) {
    if (!timerWakeUs && !powerKeyReady) {
        Serial.println("P4 sleep rejected: PMIC key events unavailable");
        return;
    }
    uint32_t started=millis();
    uint32_t beforeGame=gameClock.milliseconds(started);
    uint8_t stale=0;
    if (!timerWakeUs && !readPowerKey(stale)) {
        Serial.println("P4 sleep rejected: cannot acknowledge PMIC key");
        gameClock.exclude(millis()-started);
        return;
    }
    powerButton.arm(started);
    recordPower("sleep-request");
    uint32_t beforeCrc=esp_rom_crc32_le(0,(const uint8_t *)scenePixels,p4::GameW*p4::GameH*2);
    controls.cancel();
    auto releases=keyRepeater.update(0,started);
    const PALKEY heldKeys[]={kKeyUp,kKeyDown,kKeyLeft,kKeyRight,kKeyPgUp,kKeyPgDn};
    for (auto key:heldKeys) if (releases.released&key) PAL_KeyUp(key);
    PAL_ClearKeyState();
    drawOverlay();
    presentRect({0,p4::GameTop,p4::ScreenW,p4::GameBottom-p4::GameTop});
    if (audioTask) {
        while (xSemaphoreTake(audioParked,0)==pdTRUE) {}
        audioParkRequested.store(true,std::memory_order_release);
        if (xSemaphoreTake(audioParked,pdMS_TO_TICKS(500))!=pdTRUE) {
            audioParkRequested.store(false,std::memory_order_release);
            xTaskNotifyGive(audioTask);
            gameClock.exclude(millis()-started);
            Serial.println("P4 sleep rejected: audio worker did not park");
            return;
        }
    }
    bool hadAudio=audioReady;
    if (codec) es8311_voice_mute(codec,true);
    digitalWrite(53,LOW);
    if (codec) {
        esp_err_t codecResult=es8311_suspend(codec);
        Serial.printf("P4 sleep: codec suspend=%s\n",esp_err_to_name(codecResult));
    }
    if (hadAudio) i2s_stop(I2S_NUM_0);
    digitalWrite(28,LOW);
    lcd.displayOff();
    esp_err_t result=esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO,ESP_PD_OPTION_ON);
    Serial.printf("P4 sleep: entering light sleep timer=%llu us\n",(unsigned long long)timerWakeUs);
    Serial.flush();
    esp_sleep_wakeup_cause_t cause=ESP_SLEEP_WAKEUP_UNDEFINED;
    const int64_t deadline=esp_timer_get_time()+(int64_t)timerWakeUs;
    unsigned failures=0;
    // The PMIC IRQ pin is not wired to the MCU. Poll its latched short-press
    // flag after 500 ms light-sleep intervals, leaving screen/audio parked.
    // Neither GPIO49 noise nor touchscreen IRQs can restore the game.
    while (result==ESP_OK) {
        int64_t remaining=deadline-esp_timer_get_time();
        if (timerWakeUs && remaining<=0) break;
        uint64_t interval=timerWakeUs && remaining<500000 ? remaining : 500000;
        result=esp_sleep_enable_timer_wakeup(interval);
        if (result==ESP_OK) result=esp_light_sleep_start();
        cause=esp_sleep_get_wakeup_cause();
        if (result!=ESP_OK) break;
        if (!timerWakeUs) {
            uint8_t irq=0;
            if (!readPowerKey(irq)) {
                if (++failures>=3) { result=ESP_FAIL; break; }
            } else {
                failures=0;
                if (powerButton.consume(irq,millis())) break;
            }
        }
        delay(1); // Let idle tasks run between repeated retention intervals.
    }
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    lcd.displayOn();
    lcd.setRotation(p4::Rotation);
    // Keep contacts and the waking PWR press from becoming game input.
    readPowerKey(stale);
    powerButton.arm(millis());
    lastTouchPoll=lastTouchSuccess=millis();
    drawOverlay();
    presentRect({0,0,p4::ScreenW,p4::ScreenH});
    digitalWrite(28,HIGH);
    if (hadAudio) {
        esp_err_t audioResult=i2s_start(I2S_NUM_0);
        if (audioResult==ESP_OK) audioResult=i2s_zero_dma_buffer(I2S_NUM_0);
        if (audioResult==ESP_OK) audioResult=configureCodec();
        audioReady=audioResult==ESP_OK;
        digitalWrite(53,audioReady?HIGH:LOW);
        if (audioResult!=ESP_OK) Serial.printf("P4 audio resume failed: %s\n",esp_err_to_name(audioResult));
    }
    recordPower(timerWakeUs?"wake-test":"wake-pwr");
    uint32_t elapsed=millis()-started;
    gameClock.exclude(elapsed);
    uint32_t afterGame=UTIL_GetMilliseconds();
    uint32_t afterCrc=esp_rom_crc32_le(0,(const uint8_t *)scenePixels,p4::GameW*p4::GameH*2);
    if (audioTask) {
        audioParkRequested.store(false,std::memory_order_release);
        xTaskNotifyGive(audioTask);
    }
    Serial.printf("P4 sleep: resumed result=%s cause=%d paused=%lu ms game_delta=%lu ms PSRAM=%s volume=%d/5\n",
        esp_err_to_name(result),(int)cause,(unsigned long)elapsed,
        (unsigned long)(afterGame-beforeGame),beforeCrc==afterCrc?"OK":"CHANGED",volumeLevel);
}

static bool powerEvent(uint32_t now) {
    if (powerKeyReady && now-lastKeyPoll>=50) {
        lastKeyPoll=now;
        uint8_t irq=0;
        if (readPowerKey(irq) && powerButton.consume(irq,now)) {
            sleepGame(); return true;
        }
    }
    // Exact UART maintenance command exercises real sleep/retention without
    // needing to synthesize a physical key press; normal play has no timer wake.
    static char command[24];
    static unsigned length=0;
    while (Serial.available()) {
        char c=(char)Serial.read();
        if (c=='\r') continue;
        if (c=='\n') {
            command[length]=0; length=0;
            if (!strcmp(command,"P4POWERLOG")) {
                uint32_t started=millis();
                Serial.println("P4 POWER LOG BEGIN");
                File log=SD_MMC.open("/p4-power.log",FILE_READ);
                uint8_t buffer[256];
                while (log && log.available()) {
                    size_t n=log.read(buffer,sizeof(buffer));
                    if (!n) break;
                    Serial.write(buffer,n);
                    delay(1);
                }
                log.close();
                Serial.println("P4 POWER LOG END");
                gameClock.exclude(millis()-started);
                return true;
            }
            if (!strcmp(command,"P4POWER")) {
                recordPower("uart-status"); return true;
            }
            if (!strcmp(command,"P4SLEEPTEST")) { sleepGame(2000000); return true; }
            unsigned long duration=0;
            char extra;
            if (sscanf(command,"P4SLEEPTEST %lu%c",&duration,&extra)==1 && duration>=100 && duration<=30000) {
                sleepGame((uint64_t)duration*1000); return true;
            }
        } else if (length<sizeof(command)-1) command[length++]=c;
        else length=0;
    }
    return false;
}

extern "C" int DRIVER_Process_Events(void) {
    uint32_t now=millis();
    if (gameReady && powerEvent(now)) return 0;
    if (now-lastTouchPoll<12) return 0;
    lastTouchPoll=now;
    auto old=controls.state;
    p4::Point points[2];
    int active=0;
    bool valid=false;
    Wire.beginTransmission(0x38); Wire.write(0x02);
    if (Wire.endTransmission(false)==0 && Wire.requestFrom(0x38,13)==13) {
        uint8_t raw[13]; for (auto &v:raw) v=Wire.read();
        unsigned count=raw[0]&0x0f;
        valid=count<=2;
        if (valid) for (unsigned i=0;i<count;++i) {
            const uint8_t *p=raw+1+i*6;
            if ((p[0]>>6)==1) continue;
            int x=((p[0]&0xf)<<8)|p[1], y=((p[2]&0xf)<<8)|p[3];
            if (x>=320 || y>=480) { valid=false; break; }
            points[active++]=p4::landscapeTouch(x,y,p[2]>>4);
        }
    }
    if (valid) {
        lastTouchSuccess=now;
        controls.update(points,active,now);
    } else if (now-lastTouchSuccess>100) controls.cancel();
    // A failed read must not replay the previous poll's one-shot commands.
    else controls.state.commands=0;
    applyRequestedVolume();
    bool powerChanged=refreshPower(!old.toolbar && controls.state.toolbar,now);
    auto keyEvents=keyRepeater.update(controls.state.keys,now);
    const PALKEY heldKeys[]={kKeyUp,kKeyDown,kKeyLeft,kKeyRight,kKeyPgUp,kKeyPgDn};
    for (auto key:heldKeys) if (keyEvents.released&key) PAL_KeyUp(key);
    for (auto key:heldKeys) if (keyEvents.pressed&key) PAL_KeyDown(key);
    if (controls.state.commands) {
        PAL_KeyDown((PALKEY)controls.state.commands);
        PAL_KeyUp((PALKEY)controls.state.commands);
        Serial.printf("P4 touch command: 0x%lx\n",(unsigned long)controls.state.commands);
    }
    repaintControls(old);
    if (powerChanged && old.toolbar && controls.state.toolbar) {
        drawOverlay();
        presentRect({0,p4::Toolbar.y,p4::ScreenW,20});
    }
    return 0;
}

void PAL_P4_Setup() {
    Serial.setRxBufferSize(8192);
    Serial.begin(921600, SERIAL_8N1, 38, 37);
    delay(300);
    Serial.println("SDLPAL Waveshare LCD35 P4 boot");
    pinMode(49,INPUT);
    Serial.printf("P4 PWR: GPIO49 idle=%d (pressed=HIGH)\n",digitalRead(49));
    Wire.begin(7, 8, 400000);
    Wire.setTimeOut(30);
    refreshPower(true,millis());
    pinMode(29, OUTPUT); // FT6336 reset is separate from the LCD reset.
    digitalWrite(29, LOW); delay(20);
    digitalWrite(29, HIGH); delay(200);
    pinMode(50, INPUT);
    for (uint8_t address = 8; address < 120; ++address) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) Serial.printf("I2C device: 0x%02x\n", address);
    }
    pinMode(28, OUTPUT); digitalWrite(28, LOW);
    displayReady = lcd.begin(80000000);
    if (!displayReady) { Serial.println("LCD init failed"); return; }
    lcd.fillScreen(0);
    if (!psramFound()) { status("PSRAM required\nEnable PSRAM"); return; }
    scenePixels=(uint16_t *)heap_caps_calloc(p4::GameW*p4::GameH,sizeof(uint16_t),
                                            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!scenePixels || !overlay.begin()) { status("Display memory failed"); return; }
    drawOverlay();
    uint32_t before=micros();
    presentRect({0,0,p4::ScreenW,p4::ScreenH},true);
    Serial.printf("P4 landscape: 480x300 centered, USB right, rotation=%d frame=%lu us\n",
                  p4::Rotation,(unsigned long)(micros()-before));
    digitalWrite(28,HIGH);
    Wire.beginTransmission(0x38);
    Serial.printf("FT6336 touch: %s\n", Wire.endTransmission() == 0 ? "detected at 0x38" : "NOT DETECTED");
    Serial.printf("PSRAM: %u bytes\n", (unsigned)ESP.getPsramSize());
    if (!mountCard()) { status("SD mount failed\nInsert FAT32 card"); return; }
    powerLogReady=true;
    recordPower("boot");
    Serial.printf("SD: %llu MB, used %llu MB\n", SD_MMC.cardSize() / 1048576, SD_MMC.usedBytes() / 1048576);
    status("SDLPAL ready\nChecking game data");
    Serial.println("Send PALSD within 5 seconds for SD transfer");
    uint32_t deadline = millis() + 5000;
    while ((int32_t)(millis() - deadline) < 0) {
        if (Serial.available()) {
            String command = Serial.readStringUntil('\n'); command.trim();
            Serial.printf("UART command: [%s]\n", command.c_str());
            if (command == "PALSD") serveCard();
        }
        delay(1);
    }
    if (!dataReady()) serveCard();
    PAL_Init();
    initPowerKey();
    gameReady = true;
    Serial.println("PAL GAME READY");
    PAL_TrademarkScreen();
    PAL_SplashScreen();
    PAL_OpeningMenu();
    Serial.println("PAL GAME RUNNING");
}
void PAL_P4_Loop() {
    if (!gameReady) { delay(100); return; }
    // PAL_StartFrame already waits FRAME_TIME and handles input. Waiting here
    // again halves walking speed and discards action taps during that interval.
    PAL_StartFrame();
}
#endif
