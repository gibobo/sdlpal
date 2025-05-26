#include "../src/input.h"
#include "DrvIf_internal.h"
#include "mongoose.h"
#include <stdint.h>

struct mg_connection *ws_conn = NULL;
struct mg_connection *nc = NULL;
struct mg_mgr mgr;
unsigned char rgdwKeyLastTime[20] = {0};
/*
static const int g_KeyMap[][2] = {
    {"ArrowUp", kKeyUp},
    {"8", kKeyUp},
    {"ArrowDown", kKeyDown},
    {"2", kKeyDown},
    {"ArrowLeft", kKeyLeft},
    {"4", kKeyLeft},
    {"ArrowRight", kKeyRight},
    {"6", kKeyRight},
    {"Escape", kKeyMenu},
    {"Insert", kKeyMenu},
    {"Alt", kKeyMenu},
    {"0", kKeyMenu},
    {"Enter", kKeySearch},
    {" ", kKeySearch},
    {"Control", kKeySearch},
    {"PageUp", kKeyPgUp},
    {"9", kKeyPgUp},
    {"PageDown", kKeyPgDn},
    {"3", kKeyPgDn},
    {"Home", kKeyHome},
    {"7", kKeyHome},
    {"End", kKeyEnd},
    {"1", kKeyEnd},
    {"r", kKeyRepeat},
    {"a", kKeyAuto},
    {"d", kKeyDefend},
    {"e", kKeyUseItem},
    {"w", kKeyThrowItem},
    {"q", kKeyFlee},
    {"f", kKeyForce},
    {"s", kKeyStatus}};
    */
void handle_input(const char *data, size_t len)
{
    const char keyType = data[0];
    const char *keyCode = (const char *)data + 1;
    int Key = 0, i = 0;
    if (strstr(keyCode, "Escape") || strstr(keyCode, "Insert") || strstr(keyCode, "Alt") || strstr(keyCode, "0"))
        Key = kKeyMenu, i = 0;
    else if (strstr(keyCode, "End") || strstr(keyCode, "1"))
        Key = kKeyEnd, i = 9;
    else if (strstr(keyCode, "Enter") || strstr(keyCode, " "))
        Key = kKeySearch, i = 5;
    else if (strstr(keyCode, "ArrowUp") || strstr(keyCode, "8"))
        Key = kKeyUp, i = 1;
    else if (strstr(keyCode, "ArrowDown") || strstr(keyCode, "2"))
        Key = kKeyDown, i = 2;
    else if (strstr(keyCode, "ArrowLeft") || strstr(keyCode, "4"))
        Key = kKeyLeft, i = 3;
    else if (strstr(keyCode, "ArrowRight") || strstr(keyCode, "6"))
        Key = kKeyRight, i = 4;
    else if (strstr(keyCode, "PageUp") || strstr(keyCode, "9"))
        Key = kKeyPgUp, i = 6;
    else if (strstr(keyCode, "PageDown") || strstr(keyCode, "3"))
        Key = kKeyPgDn, i = 7;
    else if (strstr(keyCode, "Home") || strstr(keyCode, "7"))
        Key = kKeyHome, i = 8;
    else if (strstr(keyCode, "r") || strstr(keyCode, "R"))
        Key = kKeyRepeat, i = 10;
    else if (strstr(keyCode, "a") || strstr(keyCode, "A"))
        Key = kKeyAuto, i = 11;
    else if (strstr(keyCode, "d") || strstr(keyCode, "D"))
        Key = kKeyDefend, i = 12;
    else if (strstr(keyCode, "e") || strstr(keyCode, "E"))
        Key = kKeyUseItem, i = 13;
    else if (strstr(keyCode, "w") || strstr(keyCode, "W"))
        Key = kKeyThrowItem, i = 14;
    else if (strstr(keyCode, "q") || strstr(keyCode, "Q"))
        Key = kKeyFlee, i = 15;
    else if (strstr(keyCode, "f") || strstr(keyCode, "F"))
        Key = kKeyForce, i = 16;
    else if (strstr(keyCode, "s") || strstr(keyCode, "S"))
        Key = kKeyStatus, i = 17;
    printf("Received key %d: %s\n", keyType - '0', keyCode);
    if (keyType == '0')
    {
        PAL_KeyDown(Key, (rgdwKeyLastTime[i] != 0));
        rgdwKeyLastTime[i] = 0xFF;
    }
    else if (keyType == '2')
    {
        PAL_KeyUp(Key);
        memset(rgdwKeyLastTime, 0, sizeof(rgdwKeyLastTime));
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
    mg_mgr_poll(&mgr, 1);
    return 0;
}