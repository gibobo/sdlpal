#include "../src/audio.h"
#include "../src/input.h"
#include "../src/util.h"
#include "DrvIf_internal.h"
#include "mongoose.h"
#include <stdint.h>

struct mg_connection *ws_conn = NULL;
struct mg_connection *nc = NULL;
struct mg_mgr mgr;
static long rgdwKeyLastTime[20] = {0};

extern void DRIVER_UpdatePalette(const unsigned char *rgPalette);
extern void send_audio_config();
extern void generate_audio(void);
extern void clear_audio(void);

unsigned char KeyCompare(const char *Key1, const size_t Key1_len, const char *Key2)
{
    return (strlen(Key2) == Key1_len - 1) && (strstr(Key1 + 1, Key2) != NULL);
}

void handle_input(const char *data, size_t len)
{
    int Key = 0;
    unsigned char i = 0xff; // Default to invalid key
    if (KeyCompare(data, len, "Escape") || KeyCompare(data, len, "Insert") || KeyCompare(data, len, "Alt") || KeyCompare(data, len, "0"))
        Key = kKeyMenu, i = 0;
    else if (KeyCompare(data, len, "Enter") || KeyCompare(data, len, " "))
        Key = kKeySearch, i = 1;
    else if (KeyCompare(data, len, "ArrowDown") || KeyCompare(data, len, "2"))
        Key = kKeyDown, i = 2;
    else if (KeyCompare(data, len, "ArrowLeft") || KeyCompare(data, len, "4"))
        Key = kKeyLeft, i = 3;
    else if (KeyCompare(data, len, "ArrowUp") || KeyCompare(data, len, "8"))
        Key = kKeyUp, i = 4;
    else if (KeyCompare(data, len, "ArrowRight") || KeyCompare(data, len, "6"))
        Key = kKeyRight, i = 5;
    else if (KeyCompare(data, len, "PageUp") || KeyCompare(data, len, "9"))
        Key = kKeyPgUp, i = 6;
    else if (KeyCompare(data, len, "PageDown") || KeyCompare(data, len, "3"))
        Key = kKeyPgDn, i = 7;
    else if (KeyCompare(data, len, "r") || KeyCompare(data, len, "R"))
        Key = kKeyRepeat, i = 8;
    else if (KeyCompare(data, len, "a") || KeyCompare(data, len, "A"))
        Key = kKeyAuto, i = 9;
    else if (KeyCompare(data, len, "d") || KeyCompare(data, len, "D"))
        Key = kKeyDefend, i = 10;
    else if (KeyCompare(data, len, "e") || KeyCompare(data, len, "E"))
        Key = kKeyUseItem, i = 11;
    else if (KeyCompare(data, len, "w") || KeyCompare(data, len, "W"))
        Key = kKeyThrowItem, i = 12;
    else if (KeyCompare(data, len, "q") || KeyCompare(data, len, "Q"))
        Key = kKeyFlee, i = 13;
    else if (KeyCompare(data, len, "s") || KeyCompare(data, len, "S"))
        Key = kKeyStatus, i = 14;
    else if (KeyCompare(data, len, "f") || KeyCompare(data, len, "F"))
        Key = kKeyForce, i = 15;
    else if (KeyCompare(data, len, "Home") || KeyCompare(data, len, "7"))
        Key = kKeyHome, i = 16;
    else if (KeyCompare(data, len, "End") || KeyCompare(data, len, "1"))
        Key = kKeyEnd, i = 17;
    // printf("Received key %d: %s\n", keyType - '0', keyCode);
    if (i == 0xff)
        return;

    if (data[0] == '0' || data[0] == '1')
    {
        PAL_KeyDown(1 << i, rgdwKeyLastTime[i]);
        rgdwKeyLastTime[i] = UTIL_GetTicks();
    }
    else if (data[0] == '2')
    {
        PAL_KeyUp(1 << i);
        rgdwKeyLastTime[i] = 0;
    }
}

void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
    if (ev == MG_EV_HTTP_MSG)
    {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;

        if (mg_match(hm->uri, mg_str("/"), NULL))
        {
            struct mg_http_serve_opts opts = {0};
            mg_http_serve_file(nc, hm, "index.html", &opts);
        }
        else if (mg_match(hm->uri, mg_str("/ws"), NULL))
        {
            mg_ws_upgrade(nc, hm, NULL);
            ws_conn = nc;
            DRIVER_UpdatePalette(NULL); // Send palette on connect
            send_audio_config();        // Send audio config on connect
        }
    }
    else if (ev == MG_EV_WS_MSG)
    {
        struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
        if (wm->data.len > 0)
        {
            if ((int)wm->data.buf[0] >= '0' && (int)wm->data.buf[0] <= '2')
                handle_input(wm->data.buf, wm->data.len);
        }
    }
    else if (ev == MG_EV_CLOSE)
    {
        if (nc == ws_conn)
        {
            ws_conn = NULL;
        }
    }
}

int DRIVER_Init_Event(void)
{
    mg_log_set(MG_LL_ERROR);
    mg_mgr_init(&mgr);
    nc = mg_http_listen(&mgr, "http://localhost:8000", ev_handler, NULL);
    if (nc == NULL)
    {
        fprintf(stderr, "Failed to start server\n");
        return -1;
    }
    fprintf(stdout, "Starting web server on port 8000\n");
    memset(rgdwKeyLastTime, 0, sizeof(rgdwKeyLastTime));
    return 0;
}

void DRIVER_DeInit_Event(void)
{
    mg_mgr_free(&mgr);
}

int DRIVER_Process_Events(void)
{
    static unsigned long last_tick = 0;
    while (ws_conn == NULL) // Wait for WebSocket connection to be established
    {
        clear_audio();
        mg_mgr_poll(&mgr, 1000);
    }

    if (UTIL_GetTicks() >= last_tick) // Poll the event manager every 10 milliseconds
    {
        generate_audio();                 // Generate audio data
        mg_mgr_poll(&mgr, 0);             // Poll the event manager for events
        last_tick = UTIL_GetTicks() + 10; // Adjust the tick interval based on audio settings
    }
    return 0;
}