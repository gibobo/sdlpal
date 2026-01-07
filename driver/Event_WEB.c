#include "../src/audio.h"
#include "../src/driver.h"
#include "../src/input.h"
#include "../src/util.h"
#include "../src/video.h"
#include "DrvIf_internal.h"
#include "mongoose.h"
#include <stdint.h>

struct mg_connection *ws_conn = NULL;
struct mg_connection *listener_conn = NULL;
struct mg_mgr mgr;

extern void send_video_frame(void);
extern void send_audio_config(void);
extern void send_audio_data(void);

void handle_input(const char *data, size_t len);
void ev_handler(struct mg_connection *nc, int32_t ev, void *ev_data);

typedef struct
{
    const char *name;
    PALKEY keycode;
} KeyMapEntry;

static const KeyMapEntry key_map[] = {
    {"Escape", kKeyMenu},
    {"Insert", kKeyMenu},
    {"Alt", kKeyMenu},
    {"0", kKeyMenu},
    {"Enter", kKeySearch},
    {" ", kKeySearch},
    {"ArrowDown", kKeyDown},
    {"2", kKeyDown},
    {"ArrowLeft", kKeyLeft},
    {"4", kKeyLeft},
    {"ArrowUp", kKeyUp},
    {"8", kKeyUp},
    {"ArrowRight", kKeyRight},
    {"6", kKeyRight},
    {"PageUp", kKeyPgUp},
    {"9", kKeyPgUp},
    {"PageDown", kKeyPgDn},
    {"3", kKeyPgDn},
    {"r", kKeyRepeat},
    {"R", kKeyRepeat},
    {"a", kKeyAuto},
    {"A", kKeyAuto},
    {"d", kKeyDefend},
    {"D", kKeyDefend},
    {"e", kKeyUseItem},
    {"E", kKeyUseItem},
    {"w", kKeyThrowItem},
    {"W", kKeyThrowItem},
    {"q", kKeyFlee},
    {"Q", kKeyFlee},
    {"s", kKeyStatus},
    {"S", kKeyStatus},
    {"f", kKeyForce},
    {"F", kKeyForce},
    {"Home", kKeyHome},
    {"7", kKeyHome},
    {"End", kKeyEnd},
    {"1", kKeyEnd},
};

static int match_key(const char *data, size_t len, const char *key)
{
    size_t key_len = strlen(key);
    return (len == key_len + 1) && (strncmp(data + 1, key, key_len) == 0);
}

void handle_input(const char *data, size_t len)
{
    for (size_t i = 0; i < sizeof(key_map) / sizeof(key_map[0]); ++i)
    {
        if (match_key(data, len, key_map[i].name))
        {
            if (data[0] == '0')
                PAL_KeyDown(key_map[i].keycode);
            else
                PAL_KeyUp(key_map[i].keycode);
            return;
        }
    }
}

void ev_handler(struct mg_connection *nc, int32_t ev, void *ev_data)
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
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            char time_str[32];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
            fprintf(stdout, "[%s] WebSocket message received: %.*s\n", time_str, (int)wm->data.len, wm->data.buf);
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
    listener_conn = mg_http_listen(&mgr, "http://0.0.0.0:8000", ev_handler, NULL);
    if (listener_conn == NULL)
    {
        fprintf(stderr, "Failed to start server\n");
        return -1;
    }
    fprintf(stdout, "Starting web server on port 8000\n");
    return 0;
}

void DRIVER_DeInit_Event(void)
{
    mg_mgr_free(&mgr);
}

int DRIVER_Process_Events(void)
{
    static uint32_t video_trigger_ticks = 0;
    static uint32_t audio_trigger_ticks = 0;
    uint32_t current_time;

    while (ws_conn == NULL) // Wait for WebSocket connection to be established
    {
        mg_mgr_poll(&mgr, 1000);
    }

    current_time = UTIL_GetMilliseconds();
#define TRIGGER_TIME(tm) (tm * ((current_time / tm) + 1U)) // Helper macro to adjust trigger time
    if (current_time >= video_trigger_ticks)               // Poll the event manager every 50 milliseconds
    {
        send_video_frame();                      // Generate and send video frame
        mg_mgr_poll(&mgr, 0);                    // Poll the event manager for events
        video_trigger_ticks = TRIGGER_TIME(50U); // Adjust the tick interval based on video settings
    }
    if (current_time >= audio_trigger_ticks) // Poll the event manager every 10 milliseconds
    {
        send_audio_data();                       // Send audio data if available
        mg_mgr_poll(&mgr, 0);                    // Poll the event manager for events
        audio_trigger_ticks = TRIGGER_TIME(10U); // Adjust the tick interval based on audio settings
    }
#undef TRIGGER_TIME
    return 0;
}
