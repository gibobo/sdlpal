#include "../../backend_select.h"
#if defined(ARDUINO_ARCH_ESP32) && PAL_ESP32_BACKEND_HW
#include "ili9341_lib.h"
#include "esp32_pins.h"
#include <Arduino.h>
#include <SPI.h>
#include <stdlib.h>

// ILI9341 指令集
#define ILI9341_SWRESET 0x01
#define ILI9341_SLPOUT  0x11
#define ILI9341_DISPON  0x29
#define ILI9341_DISPOFF 0x28
#define ILI9341_CASET   0x2A
#define ILI9341_PASET   0x2B
#define ILI9341_RAMWR   0x2C
#define ILI9341_MADCTL  0x36
#define ILI9341_PIXFMT  0x3A

// 內部螢幕解析度參數，預設為直向 (240×320)
static uint16_t tft_width = ILI9341_TFT_WIDTH;
static uint16_t tft_height = ILI9341_TFT_HEIGHT;
static unsigned char device_init_flag = 0;

// 低階 SPI 傳送函式
static void ili9341_send_command(uint8_t cmd)
{
    digitalWrite(TFT_DC_PIN, LOW); // 指令模式
    digitalWrite(TFT_CS_PIN, LOW);
    SPI.beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
    SPI.transfer(cmd);
    SPI.endTransaction();
    digitalWrite(TFT_CS_PIN, HIGH);
}

static void ili9341_send_data(uint8_t data)
{
    digitalWrite(TFT_DC_PIN, HIGH); // 資料模式
    digitalWrite(TFT_CS_PIN, LOW);
    SPI.beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
    SPI.transfer(data);
    SPI.endTransaction();
    digitalWrite(TFT_CS_PIN, HIGH);
}

static void tft_data16(uint16_t data)
{
    digitalWrite(TFT_DC_PIN, HIGH); // 資料模式
    digitalWrite(TFT_CS_PIN, LOW);
    SPI.beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
    SPI.transfer(data >> 8);
    SPI.transfer(data & 0xFF);
    SPI.endTransaction();
    digitalWrite(TFT_CS_PIN, HIGH);
}

// 設定繪圖區域 (從 (x0,y0) 到 (x1,y1))
static void ILI9341_setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    ili9341_send_command(ILI9341_CASET); // 設定 column address
    tft_data16(x0);
    tft_data16(x1);

    ili9341_send_command(ILI9341_PASET); // 設定 row address
    tft_data16(y0);
    tft_data16(y1);

    ili9341_send_command(ILI9341_RAMWR); // 寫入記憶體
}

// 初始化螢幕
void ILI9341_init(void)
{
    if (device_init_flag)
        return;
    // 設定控制腳為輸出
    pinMode(TFT_CS_PIN, OUTPUT);
    pinMode(TFT_DC_PIN, OUTPUT);
    pinMode(TFT_RST_PIN, OUTPUT);
    pinMode(TFT_LED_PIN, OUTPUT);

    // 硬體重置
    digitalWrite(TFT_RST_PIN, LOW);
    delay(50);
    digitalWrite(TFT_RST_PIN, HIGH);
    delay(50);

    // 初始化 SPI（MISO 不用，因此設為 -1）
    SPI.begin(TFT_CLK_PIN, -1, TFT_MOSI_PIN, TFT_CS_PIN);

    // 軟體重置
    ili9341_send_command(ILI9341_SWRESET);
    delay(100);

    ili9341_send_command(ILI9341_DISPOFF);
    ili9341_send_command(ILI9341_PIXFMT);
    ili9341_send_data(0x55); // 16 位像素 (RGB565)

    // 設定為橫向模式 (解析度變為 320×240)
    ILI9341_setRotation(1);

    // 喚醒
    ili9341_send_command(ILI9341_SLPOUT);
    delay(100);

    // 開啟顯示
    ili9341_send_command(ILI9341_DISPON);

    // 開啟背光
    digitalWrite(TFT_LED_PIN, HIGH);
    ILI9341_fillScreen(0);
    device_init_flag = 1;
}

// 設定螢幕旋轉，m = 0~3
// 預設 m=1 為橫向模式 (320×240)
void ILI9341_setRotation(uint8_t m)
{
    ili9341_send_command(ILI9341_MADCTL);
    switch (m)
    {
        case 0:
            ili9341_send_data(0x48); // 直向
            tft_width = 240;
            tft_height = 320;
            break;
        case 1:
            ili9341_send_data(0x28); // 橫向
            tft_width = 320;
            tft_height = 240;
            break;
        case 2:
            ili9341_send_data(0x88); // 直向翻轉
            tft_width = 240;
            tft_height = 320;
            break;
        case 3:
            ili9341_send_data(0xE8); // 橫向翻轉
            tft_width = 320;
            tft_height = 240;
            break;
        default:
            // 預設為直向
            ili9341_send_data(0x48);
            tft_width = 240;
            tft_height = 320;
            break;
    }
}

// 填滿整個螢幕 (RGB565 格式)
void ILI9341_fillScreen(uint16_t color)
{
    if (device_init_flag == 0)
        return;
    ILI9341_setAddressWindow(0, 0, tft_width - 1, tft_height - 1);

    digitalWrite(TFT_DC_PIN, HIGH); // 資料模式
    digitalWrite(TFT_CS_PIN, LOW);
    SPI.beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
    uint32_t pixels = (uint32_t)tft_width * tft_height;
    for (uint32_t i = 0; i < pixels; i++)
    {
        SPI.transfer(color >> 8);
        SPI.transfer(color & 0xFF);
    }
    SPI.endTransaction();
    digitalWrite(TFT_CS_PIN, HIGH);
}

// 將緩衝區內容顯示至螢幕上 (RGB565)
// 假設影像解析度固定為 320×200，置中顯示於螢幕上
void ILI9341_showFrameBuffer(uint16_t *buffer)
{
    if (buffer == NULL || device_init_flag == 0)
        return;

    // 計算水平與垂直置中位置
    uint16_t startX = (tft_width > VIDEO_SCREEN_W) ? ((tft_width - VIDEO_SCREEN_W) / 2) : 0;
    uint16_t startY = (tft_height > VIDEO_SCREEN_H) ? ((tft_height - VIDEO_SCREEN_H) / 2) : 0;

    ILI9341_setAddressWindow(startX, startY, startX + VIDEO_SCREEN_W - 1, startY + VIDEO_SCREEN_H - 1);

    digitalWrite(TFT_DC_PIN, HIGH);
    digitalWrite(TFT_CS_PIN, LOW);
    SPI.beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
    // 直接傳送整個緩衝區資料 (每個像素已是 RGB565 格式)
    for (uint32_t i = 0; i < VIDEO_SCREEN_W * VIDEO_SCREEN_H; i++)
    {
        uint16_t color = buffer[i];
        SPI.transfer(color >> 8);
        SPI.transfer(color & 0xFF);
    }
    SPI.endTransaction();
    digitalWrite(TFT_CS_PIN, HIGH);
}
#endif /* ARDUINO_ARCH_ESP32 && PAL_ESP32_BACKEND_HW */
