#include "../../backend_select.h"
#if defined(ARDUINO_ARCH_ESP32) && PAL_ESP32_BACKEND_HW

/* Copyright (c) 2020, Peter Barrett
**
** Permission to use, copy, modify, and/or distribute this software for
** any purpose with or without fee is hereby granted, provided that the
** above copyright notice and this permission notice appear in all copies.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
** SOFTWARE.
*/

#include "esp32_pins.h"
#ifndef CONFIG_IDF_TARGET_ESP32S3
#include "CompositeColorOutput.h"
#include "driver/dac.h"
#include "driver/i2s.h"
#include "driver/periph_ctrl.h"
#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "rom/lldesc.h"
#include "soc/rtc.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

#define IRE(_x)        ((uint32_t)(((_x) + 40) * 255 / 3.3 / 147.5) << 8) // 3.3V DAC
#define SYNC_LEVEL     IRE(-40)
#define BLANKING_LEVEL IRE(0)
#define BLACK_LEVEL    IRE(7.5)
#define GRAY_LEVEL     IRE(50)
#define WHITE_LEVEL    IRE(100)
#define P0             (color >> 16)
#define P1             (color >> 8)
#define P2             (color)
#define P3             (color << 8)

static uint8_t **_lines = 0;
static uint16_t DRAM_ATTR x_scale_lut[NTSC_VIDEO_WIDTH]; // Scaling lookup table
static lldesc_t DRAM_ATTR _dma_desc[2] = {0};
intr_handle_t _isr_handle;
volatile uint16_t _line_counter = 0;
int _line_width;
int _hsync; // 68
int _hsync_long;
int _hsync_short;
int _burst_start;
int _burst_width;
int _active_start;
const uint32_t *_palette = NULL;

//====================================================================================================
//====================================================================================================
// multiple of color clock, word align
#define usec(us)              (NTSC_SAMPLES_PER_CC * round((us) * NTSC_FREQUENCY))

#define HSYNC_US              4.7
#define VSYNC_SHORT_US        (HSYNC_US / 2)
#define NTSC_LINE_DURATION_US 63.55
//===================================================================================================
//===================================================================================================
// NTSC
// AA AA                // 2 pixels, 1 color clock - atari
// AA AB BB             // 3 pixels, 2 color clocks - nes
// AAA ABB BBC CCC      // 4 pixels, 3 color clocks - sms

// cc == 3 gives 684 samples per line, 3 samples per cc, 3 pixels for 2 cc
// cc == 4 gives 912 samples per line, 4 samples per cc, 2 pixels per cc

void ntsc_init()
{
    // _palette = ntsc_palette();
    _line_width = NTSC_COLOR_CLOCKS_PER_SCANLINE * NTSC_SAMPLES_PER_CC;
    uint16_t samples_per_line = usec(NTSC_LINE_DURATION_US);
    samples_per_line &= ~1; // must be even
    _active_start = usec(NTSC_SAMPLES_PER_CC == 4 ? 10.f : 10.5f);
    uint16_t hsync_samples = usec(HSYNC_US);
    _hsync = usec(HSYNC_US);
    _hsync_short = usec(VSYNC_SHORT_US);
    _hsync_long = usec(NTSC_LINE_DURATION_US - HSYNC_US);
}

// draw a line of game in NTSC
void IRAM_ATTR blit_ntsc(uint8_t *src, uint16_t *dst)
{
    uint32_t *d = (uint32_t *)dst;
    const uint32_t *p = _palette;
    uint32_t color, c;
    uint32_t mask = 0xff;
    uint16_t i = 0;
    if (NTSC_SAMPLES_PER_CC == 3)
    {
        // https://github.com/nathalislight/NCAT/blob/main/NCAT_ESP32/src/compositevideo/video.h
        // 3 pixels to 2 color clocks, 3 samples per cc, used by nes
        // could be faster with better tables: 2953 cycles ish
        // about 18% of the core at 240Mhz
        // 170 color clocks wide: not all that attractive
        // AA AB BB
        dst += 36;
        for (i = 0; i < NTSC_VIDEO_HEIGHT; i += 3)
        {
            color = p[src[i + 0 + 8] & mask];
            dst[0 ^ 1] = P0;
            dst[1 ^ 1] = P1;
            color = p[src[i + 1 + 8] & mask];
            dst[2 ^ 1] = P2;
            dst[3 ^ 1] = P0;
            color = p[src[i + 2 + 8] & mask];
            dst[4 ^ 1] = P1;
            dst[5 ^ 1] = P2;
            dst += 6;
        }
    }
    else if (NTSC_SAMPLES_PER_CC == 4)
    {
        // AAA ABB BBC CCC
        // 4 pixels, 3 color clocks, 4 samples per cc
        // each pixel gets 3 samples, 192 color clocks wide
        while (i < NTSC_VIDEO_WIDTH)
        {
            color = p[src[x_scale_lut[i++]]];
            dst[0 ^ 1] = color >> 16;
            dst[1 ^ 1] = color >> 8;
            dst[2 ^ 1] = color;
            color = p[src[x_scale_lut[i++]]];
            dst[3 ^ 1] = color << 8;
            dst[4 ^ 1] = color >> 16;
            dst[5 ^ 1] = color >> 8;
            color = p[src[x_scale_lut[i++]]];
            dst[6 ^ 1] = color;
            dst[7 ^ 1] = color << 8;
            dst[8 ^ 1] = color >> 16;
            color = p[src[x_scale_lut[i++]]];
            dst[9 ^ 1] = color >> 8;
            dst[10 ^ 1] = color;
            dst[11 ^ 1] = color << 8;
            dst += 12;
        }
    }
}

void IRAM_ATTR burst_ntsc(uint16_t *line)
{
    int i, phase;
    switch (NTSC_SAMPLES_PER_CC)
    {
        case 3:
            // 3 samples per color clock
            phase = 0.866025 * BLANKING_LEVEL / 2;
            for (i = _hsync; i < _hsync + 30; i += 6)
            {
                line[i + 1] = BLANKING_LEVEL;
                line[i + 0] = BLANKING_LEVEL + phase;
                line[i + 3] = BLANKING_LEVEL - phase;
                line[i + 2] = BLANKING_LEVEL;
                line[i + 5] = BLANKING_LEVEL + phase;
                line[i + 4] = BLANKING_LEVEL - phase;
            }
            break;
        case 4:
            // 4 samples per color clock
            for (i = _hsync; i < _hsync + 40; i += 4)
            {
                line[i + 0] = BLANKING_LEVEL + BLANKING_LEVEL / 2;
                line[i + 1] = BLANKING_LEVEL;
                line[i + 2] = BLANKING_LEVEL - BLANKING_LEVEL / 2;
                line[i + 3] = BLANKING_LEVEL;
            }
            break;
    }
}

//===================================================================================================
//===================================================================================================
void IRAM_ATTR line_sync(uint16_t *line, int syncwidth)
{
    for (int i = 0; i < syncwidth; i++)
        line[i] = SYNC_LEVEL;
}

void IRAM_ATTR blanking(uint16_t *line, int syncwidth)
{
    for (int i = syncwidth; i < _line_width; i++)
        line[i] = BLANKING_LEVEL;
}

unsigned int convertRGBtoPalette(unsigned char r, unsigned char g, unsigned char b)
{
    float ofs = 23.f;        // black level
    float amp = 57.f;        // signal span
    float hue = M_PI;        // hue correction if any...
    float saturation = 1.0f; // Color saturation
    float wt = 0;
    float y = (0.299f * r + 0.587f * g + 0.114f * b) / 255.f;
    float u = (-0.147407f * r - 0.289391f * g + 0.436798f * b) / 255.f;
    float v = (0.614777f * r - 0.514799f * g - 0.099978f * b) / 255.f;
    unsigned int sampl = 0;
    for (int i = 3; i >= 0; i--)
    {
        sampl |= (unsigned int)round(ofs + amp * (y + saturation * (u * sinf(wt + hue) + v * cosf(wt + hue)))) << (i * 8);
        wt += M_PI / 2.f;
    }
    return sampl;
}

// Workhorse ISR handles audio and video updates
void IRAM_ATTR video_isr(volatile void *vbuf)
{
    if (!_lines)
        return;

    uint16_t i = _line_counter++;
    uint16_t j;
    uint16_t *buf = (uint16_t *)vbuf;
    uint16_t *dst = buf + _active_start; // 144

    if (i < NTSC_ACTIVE_LINES)
    {
        // active video
        line_sync(buf, _hsync); // 0 - 67
        burst_ntsc(buf);        // 68 - 107
        blit_ntsc(_lines[i], dst);
    }
    else if (i < (NTSC_ACTIVE_LINES + 5))
    {                           // post render/black
        line_sync(buf, _hsync); // 0 - 67
        blanking(buf, _hsync);
        burst_ntsc(buf); // no burst during vbl
    }
    else if (i < (NTSC_ACTIVE_LINES + 8))
    {                                // vsync
        line_sync(buf, _hsync_long); // 0 - 67
        blanking(buf, _hsync_long);
    }
    else
    {                           // pre render/black_hsync_long
        line_sync(buf, _hsync); // 0 - 67
        blanking(buf, _hsync);
        burst_ntsc(buf); // no burst during vbl
    }
    _line_counter %= NTSC_LINES;
}

// simple isr
static void IRAM_ATTR i2s_intr_handler_video(void *arg)
{
    if (I2S0.int_st.out_eof)
        video_isr(((lldesc_t *)I2S0.out_eof_des_addr)->buf); // get the next line of video
    // reset the interrupt
    I2S0.int_clr.val = I2S0.int_st.val;
}

static esp_err_t start_dma(int line_width, int samples_per_cc)
{
    const size_t dma_buffer_size_bytes = line_width * 2;
    printf("Computed DMA buffer size: %u\n", dma_buffer_size_bytes);
    if (dma_buffer_size_bytes >= 4092)
        return -1;

    periph_module_enable(PERIPH_I2S0_MODULE);
    // setup interrupt
    if (esp_intr_alloc(ETS_I2S0_INTR_SOURCE, ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM,
                       i2s_intr_handler_video, (void *)dma_buffer_size_bytes, &_isr_handle) != ESP_OK)
        return -1;

    // reset conf
    I2S0.conf.val = 1;
    I2S0.conf.val = 0;
    I2S0.conf.tx_right_first = 1;
    I2S0.conf.tx_mono = 1;

    I2S0.conf2.lcd_en = 1;
    I2S0.fifo_conf.tx_fifo_mod_force_en = 1;
    I2S0.sample_rate_conf.tx_bits_mod = 16; // DAC uses MSB 8 bits of 16
    I2S0.conf_chan.tx_chan_mod = 1;

    I2S0.clkm_conf.clkm_div_num = 1; // I2S clock divider’s integral value.
    I2S0.clkm_conf.clkm_div_b = 0;   // Fractional clock divider’s numerator value.
    I2S0.clkm_conf.clkm_div_a = 1;   // Fractional clock divider’s denominator value
    I2S0.sample_rate_conf.tx_bck_div_num = 1;
    I2S0.clkm_conf.clka_en = 1;     // Set this bit to enable clk_apll.
    I2S0.fifo_conf.tx_fifo_mod = 1; // 32-bit dual or 16-bit single channel data

    // Create TX DMA buffers
    const size_t DMA_BUFFER_COUNT = sizeof(_dma_desc) / sizeof(lldesc_t);
    for (size_t i = 0; i < DMA_BUFFER_COUNT; i++)
    {
        _dma_desc[i].buf = (uint8_t *)heap_caps_calloc(dma_buffer_size_bytes, sizeof(uint8_t), MALLOC_CAP_DMA);
        if (!_dma_desc[i].buf)
            return -1;

        _dma_desc[i].owner = 1;
        _dma_desc[i].eof = 1;
        _dma_desc[i].length = dma_buffer_size_bytes;
        _dma_desc[i].size = dma_buffer_size_bytes;
        _dma_desc[i].empty = (uint32_t)(i == DMA_BUFFER_COUNT - 1 ? &_dma_desc[0] : &_dma_desc[i + 1]);
    }
    I2S0.out_link.addr = (uint32_t)&_dma_desc[0];

    //  Setup up the apll: See ref 3.2.7 Audio PLL
    //  f_xtal = (int)rtc_clk_xtal_freq_get() * 1000000;
    //  f_out = xtal_freq * (4 + sdm2 + sdm1/256 + sdm0/65536); // 250 < f_out < 500
    //  apll_freq = f_out/((o_div + 2) * 2)
    //  operating range of the f_out is 250 MHz ~ 500 MHz
    //  operating range of the apll_freq is 16 ~ 128 MHz.
    //  select sdm0,sdm1,sdm2 to produce nice multiples of colorburst frequencies

    //  see calc_freq() for math: (4+a)*10/((2 + b)*2) mhz
    //  up to 20mhz seems to work ok:
    //  rtc_clk_apll_enable(1,0x00,0x00,0x4,0);   // 20mhz for fancy DDS

    // 10.7386363636 3x NTSC (10.7386398315mhz)
    // 14.3181818182 4x NTSC (14.3181864421mhz)
    switch (NTSC_SAMPLES_PER_CC)
    {
        case 3:
            rtc_clk_apll_enable(1, 0x46, 0x97, 0x4, 2);
            break; // 10.7386363636 3x NTSC (10.7386398315mhz)
        case 4:
            rtc_clk_apll_enable(1, 0x46, 0x97, 0x4, 1);
            break; // 14.3181818182 4x NTSC (14.3181864421mhz)
    }

    dac_output_enable(COMPOSITE_DAC_CHANNEL);
    dac_i2s_enable();

    // start transmission
    I2S0.conf.tx_start = 1; // start DMA!
    I2S0.int_clr.val = UINT32_MAX;
    I2S0.int_ena.out_eof = 1;
    I2S0.out_link.start = 1;
    return esp_intr_enable(_isr_handle); // start interruprs!
}

void video_init(void)
{
    // Initialize x_scale_lut lookup table
    for (uint16_t x = 0; x < NTSC_VIDEO_WIDTH; x++)
        x_scale_lut[x] = (uint16_t)x * VIDEO_SCREEN_W / NTSC_VIDEO_WIDTH;

    ntsc_init();
    // init the hardware
    // setup apll 4x NTSC or PAL colorburst rate
    start_dma(_line_width, NTSC_SAMPLES_PER_CC);
}

void sendFrameHalfResolution(unsigned char **frame) { _lines = frame; }

void sendPalette(const unsigned int *palette) { _palette = palette; }
#else
void video_init(void) {}
void sendFrameHalfResolution(unsigned char **frame) {}
void sendPalette(const unsigned int *palette) {}
unsigned int convertRGBtoPalette(unsigned char r, unsigned char g, unsigned char b) { return 0; }
#endif
#endif /* ARDUINO_ARCH_ESP32 && PAL_ESP32_BACKEND_HW */
