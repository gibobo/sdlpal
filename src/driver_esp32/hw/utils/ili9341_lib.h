#ifndef ILI9341_LIB_H
#define ILI9341_LIB_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

// 初始化螢幕
void ILI9341_init(void);

// 設定螢幕旋轉，m = 0~3；預設 m=1 為橫向模式 (320x240)
void ILI9341_setRotation(uint8_t m);

// 填滿整個螢幕指定顏色 (RGB565 格式)
void ILI9341_fillScreen(uint16_t color);

// 將緩衝區內容顯示至螢幕上 (影像置中顯示)
void ILI9341_showFrameBuffer(uint16_t *buffer);

#ifdef __cplusplus
}
#endif

#endif // ILI9341_LIB_H
