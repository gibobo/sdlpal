/*
 * SDLPAL on ESP32 - Arduino Main Entry Point
 *
 * Unified sketch for both ESP32 backends (selected at build time via
 * src/driver_esp32/backend_select.h):
 *   - PAL_ESP32_BACKEND_WEB : WiFi web-streaming backend (frame streamed to a browser).
 *   - PAL_ESP32_BACKEND_HW  : hardware backend (ILI9341 TFT / NTSC composite + Bluepad32).
 *
 * The Arduino build compiles this .ino plus everything under src/ recursively. The
 * portable game engine lives in src/*.c; the ESP32 backends live in src/driver_esp32/.
 * Desktop builds use CMake and ignore this file entirely.
 */

// ===== Backend selection (must be first so PAL_ESP32_BACKEND_* are defined below) =====
#include "src/driver_esp32/backend_select.h"

// ===== Arduino Framework =====
#include <Arduino.h>
#include <setjmp.h>
#include <stdio.h>

// ===== ESP32 System Libraries =====
#include "sdkconfig.h"
#include <dirent.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_vfs_fat.h>
#include <sys/stat.h>

// ===== ESP32 Hardware Drivers =====
#include <driver/sdmmc_host.h>
#include <driver/sdspi_host.h>
#include <driver/spi_common.h>
#include <driver/spi_master.h>
#include <sdmmc_cmd.h>

// ===== Hardware Abstraction Layer (per-backend pin map) =====
#if PAL_ESP32_BACKEND_WEB
#include "src/driver_esp32/web/utils/esp32_pins.h"
#elif PAL_ESP32_BACKEND_HW
#include "src/driver_esp32/hw/utils/esp32_pins.h"
#endif

// ===== PAL Game Engine (portable core, now at src/) =====
#include "src/audio.h"
#include "src/driver.h"
#include "src/font.h"
#include "src/global.h"
#include "src/input.h"
#include "src/main.h"
#include "src/palcommon.h"
#include "src/palette.h"
#include "src/play.h"
#include "src/res.h"
#include "src/rngplay.h"
#include "src/scene.h"
#include "src/text.h"
#include "src/ui.h"
#include "src/uigame.h"
#include "src/util.h"
#include "src/video.h"

// ===== Configuration Flags =====
// Uncomment to enable specific hardware features
#define PALGAME // Enable PAL game engine

// ===== Status LED =====
// GPIO 2 is used by TFT_DC_PIN on the hardware backend, so GPIO 22 is used for status.
#ifndef LED_BUILTIN
#define LED_BUILTIN 22
#endif

// ===== Helper Functions =====

void printMemoryStatus()
{
    size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t free_spiram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t freeHeap = esp_get_free_heap_size();
    size_t minFreeHeap = esp_get_minimum_free_heap_size();

    Serial.println("=== Memory Status ===");
    Serial.printf("Minimum ever free heap: %u bytes\n", (unsigned)minFreeHeap);
    Serial.printf("Free internal: %u bytes\n", (unsigned)free_internal);
    Serial.printf("Free SPIRAM: %u bytes\n", (unsigned)free_spiram);
    Serial.printf("Free heap: %u bytes\n", (unsigned)freeHeap);
}

/**
 * List all files and directories in the SD card
 * @param path The path to list
 */
void listSdCardContents(const char *path)
{
    Serial.println("\n========================================");
    Serial.println("SD Card Contents:");
    Serial.println("========================================");

    DIR *dir = opendir(path);
    if (!dir)
    {
        Serial.printf("Failed to open directory: %s\n", path);
        return;
    }

    struct dirent *entry;
    struct stat file_stat;
    char filepath[300];
    int fileCount = 0;
    int dirCount = 0;

    while ((entry = readdir(dir)) != NULL)
    {
        snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);

        if (stat(filepath, &file_stat) == 0)
        {
            if (S_ISDIR(file_stat.st_mode))
            {
                // Directory
                Serial.printf("[DIR]  %s\n", entry->d_name);
                dirCount++;
            }
            else
            {
                // File
                Serial.printf("[FILE] %-30s  Size: %7ld bytes\n", entry->d_name, file_stat.st_size);
                fileCount++;
            }
        }
        else
        {
            Serial.printf("[????] %s (stat failed)\n", entry->d_name);
        }
    }

    closedir(dir);

    Serial.println("========================================");
    Serial.printf("Total: %d file(s), %d folder(s)\n", fileCount, dirCount);
    Serial.println("========================================\n");
}

/**
 * Initialize the SD card and mount it at RESOURCE_PATH.
 * The mount path differs per backend / target:
 *   - web backend: SDMMC 1-bit on GPIO 39/38/40 (ESP32-S3-CAM wiring).
 *   - hw backend : SDMMC 4-bit on ESP32-S3, SDSPI (VSPI) on classic ESP32/WROVER.
 * @return true if successful, false otherwise
 */
bool initializeSdCard()
{
    sdmmc_card_t *card;
    esp_err_t ret;

    Serial.println("Initializing SD card...");

#if PAL_ESP32_BACKEND_WEB
    // ----- Web backend: SDMMC 1-bit -----
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.flags = SDMMC_HOST_FLAG_1BIT; // 1-bit mode

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;         // 1-bit mode
    slot_config.clk = GPIO_NUM_39; // CLK
    slot_config.cmd = GPIO_NUM_38; // CMD
    slot_config.d0 = GPIO_NUM_40;  // DATA0

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 20,
        .allocation_unit_size = 16 * 1024};

    ret = esp_vfs_fat_sdmmc_mount(RESOURCE_PATH, &host, &slot_config, &mount_config, &card);

#else // PAL_ESP32_BACKEND_HW
    // ----- Hardware backend: target-conditional (S3 = SDMMC 4-bit, else = SDSPI) -----
#ifdef CONFIG_IDF_TARGET_ESP32S3
    // Configure SDMMC host for ESP32-S3
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4; // 4-bit mode

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = SD_MAX_FILES,
        .allocation_unit_size = SD_ALLOCATION_UNIT};

    ret = esp_vfs_fat_sdmmc_mount(RESOURCE_PATH, &host, &slot_config, &mount_config, &card);
#else
    // Configure SDSPI host for classic ESP32 / WROVER variants
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = SD_MAX_FREQ_KHZ;
    host.flags = SDMMC_HOST_FLAG_SPI;

    Serial.printf("SD SPI Host Config: max_freq=%d kHz\n", host.max_freq_khz);

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_PIN;
    slot_config.host_id = SPI_HOST_USED;

    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = SD_MAX_FILES,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE};

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI_PIN,
        .miso_io_num = SD_MISO_PIN,
        .sclk_io_num = SD_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    // Initialize SPI bus with auto DMA channel allocation for better compatibility
    ret = spi_bus_initialize(SPI_HOST_USED, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK)
    {
        ESP_LOGE(__func__, "SPI bus initialization failed: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_vfs_fat_sdspi_mount(RESOURCE_PATH, &host, &slot_config, &mount_config, &card);
#endif // CONFIG_IDF_TARGET_ESP32S3
#endif // PAL_ESP32_BACKEND_WEB

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            Serial.println("Failed to mount filesystem. Consider setting format_if_mount_failed = true.");
        }
        else
        {
            Serial.printf("Failed to initialize SD card (%s). Check SD card connections and pull-up resistors.\n",
                          esp_err_to_name(ret));
        }
        return false;
    }

    Serial.println("SD card mounted successfully!");
    sdmmc_card_print_info(stdout, card);

    // List files and directories on SD card
    listSdCardContents(RESOURCE_PATH);

    return true;
}

/**
 * Arduino setup function - Initialize all hardware and game engine
 */
void setup()
{
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial)
    {
        ; // Wait for Serial port to be ready
    }

    setCpuFrequencyMhz(240);

    // Initialize LED pin for status indication
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW); // Start with LED off

    if (!initializeSdCard())
    {
        Serial.println("========================================");
        Serial.println("CRITICAL ERROR: SD card initialization failed!");
        Serial.println("Game cannot start without SD card access.");
        Serial.println("Please check:");
        Serial.println("  - SD card is properly inserted");
        Serial.println("  - SD card is formatted as FAT32");
        Serial.println("  - All SPI connections are secure");
        Serial.println("  - Add 10k pull-up resistors on SPI lines");
        Serial.println("  - Use shorter connection cables (<10cm)");
        Serial.println("========================================");

        // Flash LED to indicate error
        while (true)
        {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(500);
            digitalWrite(LED_BUILTIN, LOW);
            delay(500);
        }
    }

#ifdef PALGAME
    // Initialize everything (this also handles the backend's display/input/audio
    // and, on the hardware backend, the Bluepad32 connection).
    PAL_Init();
    printMemoryStatus();

    // Show the trademark screen and splash screen
    PAL_TrademarkScreen();
    PAL_SplashScreen();

    // Show the opening menu
    PAL_OpeningMenu();
#endif

    Serial.println("========================================");
    Serial.println("Setup completed successfully!");
    Serial.println("========================================\n");
    printMemoryStatus();
}

/**
 * Arduino main loop - Run the game engine frame by frame
 */
void loop()
{
#ifdef PALGAME
    // Wait for one frame duration and accept input
    UTIL_Delay(FRAME_TIME);

    // Execute main game frame logic
    PAL_StartFrame();
#else
    // PAL game engine not enabled, just wait
    delay(100);
#endif
}
