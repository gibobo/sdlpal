#include "../src/util.h"
#include "../src/video.h"
#include "mongoose.h"
#include <string.h>

static unsigned char *send_frame = NULL;
static unsigned char *send_palette = NULL;

extern struct mg_connection *ws_conn;
extern struct mg_mgr mgr;

unsigned char *DRIVER_FrameBuffer()
{
    return send_frame + 9;
}

void DRIVER_FrameShow(
    unsigned char *frame,
    const unsigned short roi_x,
    const unsigned short roi_y,
    const unsigned short roi_w,
    const unsigned short roi_h,
    const unsigned char padding_flag)
{
    unsigned short x, y;
    unsigned short roi_x2 = roi_x + roi_w;
    unsigned short roi_y2 = roi_y + roi_h;
    unsigned char *src = frame;
    unsigned char *dst = DRIVER_FrameBuffer();

    for (y = 0; y < SCREEN_H; y++)
    {
        if ((y >= roi_y) && (y < roi_y2))
        {
            for (x = 0; x < SCREEN_W; x++)
            {
                if ((x >= roi_x) && (x < roi_x2))
                {
                    dst[x] = src[x];
                }
                else if (padding_flag)
                {
                    dst[x] = 0;
                }
            }
            src += SCREEN_W;
        }
        else if (padding_flag)
            memset(dst, 0, SCREEN_W);
        else
            src += SCREEN_W;
        dst += SCREEN_W;
    }
    if (ws_conn && send_frame)
    {
        mg_mgr_poll(&mgr, 1);
        mg_ws_send(ws_conn, send_frame, 9 + SCREEN_SIZE, WEBSOCKET_OP_BINARY);
    }
}

void DRIVER_FrameResize(unsigned int width, unsigned int height)
{
}

void DRIVER_UpdatePalette(const unsigned char *rgPalette)
{
    size_t res;
    if (ws_conn && send_palette)
    {
        mg_mgr_poll(&mgr, 1);
        memcpy(send_palette + 1, rgPalette, 256 * 3);
        res = mg_ws_send(ws_conn, send_palette, 1 + 256 * 3, WEBSOCKET_OP_BINARY);
    }
}

int DRIVER_Init_Video(void)
{
    send_frame = (unsigned char *)UTIL_malloc(9 + SCREEN_SIZE);
    send_frame[0] = 0;
    send_frame[1] = (SCREEN_W >> 24) & 0xFF;
    send_frame[2] = (SCREEN_W >> 16) & 0xFF;
    send_frame[3] = (SCREEN_W >> 8) & 0xFF;
    send_frame[4] = SCREEN_W & 0xFF;
    send_frame[5] = (SCREEN_H >> 24) & 0xFF;
    send_frame[6] = (SCREEN_H >> 16) & 0xFF;
    send_frame[7] = (SCREEN_H >> 8) & 0xFF;
    send_frame[8] = SCREEN_H & 0xFF;

    send_palette = (unsigned char *)UTIL_malloc(1 + 256 * 3);
    send_palette[0] = 2;

    return 0;
}

void DRIVER_DeInit_Video(void)
{
    UTIL_free(send_frame);
    UTIL_free(send_palette);
    send_frame = NULL;
    send_palette = NULL;
}