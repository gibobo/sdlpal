#include "../src/driver.h"
#include "../src/util.h"
#include "../src/video.h"
#include "DrvIf_internal.h"
#include "mongoose.h"
#include <string.h>

unsigned char *framebuffer = NULL;
static unsigned char *send_palette = NULL;
extern struct mg_connection *ws_conn;
extern struct mg_mgr mgr;

// Function prototypes
void send_video_frame(void);

void DRIVER_FrameShow(
    unsigned char *frame,
    const unsigned short roi_x,
    const unsigned short roi_y,
    const unsigned short roi_w,
    const unsigned short roi_h,
    const unsigned char padding_flag)
{
    if (framebuffer == NULL)
        return;

    unsigned short x, y;
    unsigned short roi_x2 = roi_x + roi_w;
    unsigned short roi_y2 = roi_y + roi_h;
    unsigned char *src = frame;
    unsigned char *dst = framebuffer + 5;

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
}

void DRIVER_UpdatePalette(const unsigned char *rgPalette)
{
    if (send_palette == NULL)
        return;

    if (rgPalette)
        memcpy(send_palette + 1, rgPalette, 256 * 3);

    if (ws_conn)
        mg_ws_send(ws_conn, send_palette, 1 + 256 * 3, WEBSOCKET_OP_BINARY);
}

void send_video_frame(void)
{
    if (ws_conn == NULL || framebuffer == NULL)
        return;

    if ((int)mg_ws_send(ws_conn, framebuffer, 5 + SCREEN_SIZE, WEBSOCKET_OP_BINARY) == -1)
    {
        fprintf(stderr, "Failed to send video frame\n");
        return;
    }
}

int DRIVER_Init_Video(void)
{
    framebuffer = (unsigned char *)UTIL_malloc(5 + SCREEN_SIZE);
    framebuffer[0] = 0;
    framebuffer[1] = (SCREEN_W >> 8) & 0xFF;
    framebuffer[2] = SCREEN_W & 0xFF;
    framebuffer[3] = (SCREEN_H >> 8) & 0xFF;
    framebuffer[4] = SCREEN_H & 0xFF;

    send_palette = (unsigned char *)UTIL_malloc(1 + 256 * 3);
    send_palette[0] = 2;

    return 0;
}

void DRIVER_DeInit_Video(void)
{
    UTIL_free(framebuffer);
    UTIL_free(send_palette);
    framebuffer = NULL;
    send_palette = NULL;
}
