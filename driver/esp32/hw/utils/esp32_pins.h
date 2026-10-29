/*
 * ESP32 WROVER KIT Pin Configuration
 * 
 * This file contains all GPIO pin definitions for ESP32 WROVER KIT
 * hardware configuration used in the PAL game engine project.
 */

#ifndef ESP32_PINS_H
#define ESP32_PINS_H

#include "sdkconfig.h"
#include <driver/gpio.h>

// ===== ILI9341 Display Pins =====
// TFT display interface pins
#define TFT_MOSI_PIN                   GPIO_NUM_13 // SPI MOSI (Master Out Slave In)
#define TFT_CLK_PIN                    GPIO_NUM_14 // SPI Clock
#define TFT_CS_PIN                     GPIO_NUM_15 // Chip Select
#define TFT_RST_PIN                    GPIO_NUM_0  // Reset pin
#define TFT_DC_PIN                     GPIO_NUM_2  // Data/Command select
#define TFT_LED_PIN                    GPIO_NUM_12 // Backlight control

// ===== Audio Output Pins =====
// DAC output pins for composite video and audio
#define COMPOSITE_DAC_CHANNEL          DAC_CHANNEL_1 // DAC_CHANNEL_1(GPIO25), DAC_CHANNEL_2(GPIO26)

// ===== Hardware Configuration Constants =====
// Screen Buffer Configuration
#define VIDEO_SCREEN_W                 320 // Screen buffer width
#define VIDEO_SCREEN_H                 200 // Screen buffer height

// ILI9341 Display Resolution Configuration
#define ILI9341_TFT_WIDTH              240 // TFT physical width (portrait mode)
#define ILI9341_TFT_HEIGHT             320 // TFT physical height (portrait mode)

// NTSC Composite Video Configuration
#define NTSC_VIDEO_WIDTH               256            // NTSC video width
#define NTSC_VIDEO_HEIGHT              240            // NTSC video height
#define NTSC_COLOR_CLOCKS_PER_SCANLINE 228            // Color clocks per scanline (227.5 for NTSC)
#define NTSC_FREQUENCY                 (315.0 / 88.0) // DAC frequency in MHz
#define NTSC_LINES                     262            // Total lines per frame
#define NTSC_ACTIVE_LINES              240            // Active video lines
#define NTSC_SAMPLES_PER_CC            4              // Samples per color clock (3 or 4)

// SD Card SPI Configuration
#define SD_MAX_FREQ_KHZ                10000       // Max frequency for SD card
#define SD_MAX_TRANSFER_SIZE           4000        // Max transfer size
#define SD_MAX_FILES                   10          // Max open files
#define SD_ALLOCATION_UNIT             (16 * 1024) // Allocation unit size

// SPI Configuration
// Because the PSRAM on the ESP32 WROVER KIT uses HSPI, the SD card must use VSPI to avoid conflicts.
// The ESP32-S3 is not subject to this limitation and can use SDMMC.
// #define SPI_HOST_USED                  SPI2_HOST // Use SPI2_HOST (HSPI) - CONFLICT with PSRAM!
#define SPI_HOST_USED                  SPI3_HOST // Use SPI3_HOST (VSPI)

// Enforce SPI host selection for specific targets.
// When building for ESP32-S3, force use of SPI3_HOST.
// For other targets, allow SPI_HOST_USED to be set externally; if not set, default to SPI3_HOST.
#ifdef CONFIG_IDF_TARGET_ESP32
    #ifdef SPI_HOST_USED
        #undef SPI_HOST_USED
    #endif
    #define SPI_HOST_USED SPI3_HOST
#endif

// ===== SD Card SPI Pins (VSPI) =====
// SD card interface using VSPI peripheral
#if SPI_HOST_USED == SPI3_HOST
#define SD_CS_PIN   GPIO_NUM_5  // Chip Select pin
#define SD_MOSI_PIN GPIO_NUM_23 // MOSI pin (VSPI)
#define SD_MISO_PIN GPIO_NUM_19 // MISO pin (VSPI)
#define SD_SCLK_PIN GPIO_NUM_18 // SCLK pin (VSPI)
#elif SPI_HOST_USED == SPI2_HOST
#define SD_CS_PIN   GPIO_NUM_15  // Chip Select pin
#define SD_MOSI_PIN GPIO_NUM_13 // MOSI pin (HSPI)
#define SD_MISO_PIN GPIO_NUM_12 // MISO pin (HSPI)
#define SD_SCLK_PIN GPIO_NUM_14 // SCLK pin (HSPI)
#endif

#endif // ESP32_PINS_H
