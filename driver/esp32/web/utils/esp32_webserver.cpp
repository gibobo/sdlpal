#include "../../backend_select.h"
#if defined(ARDUINO_ARCH_ESP32) && PAL_ESP32_BACKEND_WEB
#include "esp32_webserver.h"
// #include "DrvIf_internal.h"
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>

// ===== WiFi / Captive Portal Configuration =====
static const byte DNS_PORT = 53;
static const char *AP_SSID = "PAL-AP";
static const IPAddress AP_IP(4, 3, 2, 1);
static const IPAddress AP_NETMASK(255, 255, 255, 0);

static DNSServer dnsServer;
static WebServer webServer(80);

// Client connection tracking
static volatile int client_connected = 0;
static uint32_t palette_version = 0;
static volatile int palette_needs_update = 0;

// Embedded HTML client: fetch /frame, decode 8-bit indices with palette, render to Canvas.
// Note: This string will be used as fallback if index.html is not found on SD card
static const char FALLBACK_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no" />
  <title>PAL on ESP32-S3</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    html, body { overscroll-behavior: none; }
    body, canvas {
      touch-action: none;
      -webkit-user-select: none;
      user-select: none;
      -webkit-touch-callout: none;
    }
    :root { color: #e4e4e4; background:#0a0a0a; font-family: "Segoe UI", sans-serif; }
    body {
      display:flex;
      flex-direction:column;
      align-items:center;
      justify-content:flex-start;
      gap:6px;
      padding:0;
      margin:0;
      min-height: 100vh;
      height: 100vh;
      width: 100vw;
      overflow: hidden;
    }
    #container {
      display: flex;
      align-items: flex-start;
      justify-content: center;
      width: 100%;
    }
    canvas {
      image-rendering: pixelated;
      image-rendering: crisp-edges;
      border:2px solid #3c3c3c;
      background:#000;
      max-width: 100vw;
      max-height: 100vh;
      display: block;
    }
    #status { font-size:14px; color:#8fb9ff; text-align: center; margin:0 6px 4px 6px; }

    /* GameBoy-like control area */
    #controls {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
      width: 100%;
      padding: 8px 12px 16px 12px;
      box-sizing: border-box;
    }
    .dpad, .buttons {
      display: grid;
      place-items: center;
    }
    .dpad-grid {
      display: grid;
      grid-template-columns: repeat(3, 52px);
      grid-template-rows: repeat(3, 52px);
      gap: 6px;
    }
    .btn {
      width: 52px; height: 52px;
      border-radius: 12px;
      border: 2px solid #3c3c3c;
      background: linear-gradient(145deg, #555, #2a2a2a);
      color: #e4e4e4;
      font-weight: bold;
      font-size: 16px;
      display: flex;
      align-items: center;
      justify-content: center;
      box-shadow: inset 0 2px 4px #111, 0 2px 4px rgba(0,0,0,0.4);
      touch-action: none;
      -webkit-touch-callout: none;
    }
    .btn.blank { visibility: hidden; }
    .buttons-stack {
      display: grid;
      grid-template-columns: repeat(2, 64px);
      grid-auto-rows: 64px;
      gap: 10px;
      justify-content: center;
    }
    .meta-row {
      margin-top: 8px;
      display: flex;
      gap: 10px;
      justify-content: center;
    }
    .meta-btn {
      min-width: 78px;
      padding: 10px 14px;
      border-radius: 10px;
      border: 2px solid #3c3c3c;
      background: linear-gradient(145deg, #666, #303030);
      color: #e4e4e4;
      font-weight: 600;
      font-size: 14px;
      box-shadow: inset 0 2px 4px #111, 0 2px 4px rgba(0,0,0,0.35);
      touch-action: none;
      -webkit-touch-callout: none;
    }
    @media (max-width: 480px) {
      .dpad-grid { grid-template-columns: repeat(3, 44px); grid-template-rows: repeat(3, 44px); }
      .btn { width: 44px; height: 44px; font-size: 14px; }
      .buttons-stack { grid-template-columns: repeat(2, 56px); grid-auto-rows: 56px; gap: 8px; }
      .meta-btn { min-width: 68px; font-size: 13px; padding: 8px 12px; }
    }
  </style>
</head>
<body>
  <div id="container">
    <canvas id="screen" width="320" height="200"></canvas>
  </div>
  <div id="status">連線中...</div>
  <div id="controls">
    <div class="dpad">
      <div class="dpad-grid">
        <div class="btn blank"></div>
        <div class="btn" data-key="up">↑</div>
        <div class="btn blank"></div>
        <div class="btn" data-key="left">←</div>
        <div class="btn blank"></div>
        <div class="btn" data-key="right">→</div>
        <div class="btn blank"></div>
        <div class="btn" data-key="down">↓</div>
        <div class="btn blank"></div>
      </div>
    </div>
    <div class="buttons">
      <div class="buttons-stack">
        <div class="btn" data-key="X">X</div>
        <div class="btn" data-key="Y">Y</div>
        <div class="btn" data-key="A">A</div>
        <div class="btn" data-key="B">B</div>
      </div>
      <div class="meta-row">
        <div class="meta-btn" data-key="Start">Start</div>
        <div class="meta-btn" data-key="Select">Select</div>
      </div>
    </div>
  </div>
  <script>
    const W=320, H=200;
    const palette=new Uint8Array(256*3);
    const canvas=document.getElementById('screen');
    const ctx=canvas.getContext('2d');
    const img=ctx.createImageData(W,H);
    const statusEl=document.getElementById('status');
    let lastVer=-1;

    // FPS 計算變數
    let frameCount = 0;
    let lastFpsUpdate = performance.now();
    let currentFps = 0;

    // 根據設備尺寸計算最佳顯示大小（保持 16:10，盡量貼齊邊界）
    function resizeCanvas() {
      const statusH = statusEl.getBoundingClientRect().height || 0;
      const controlsH = document.getElementById('controls').getBoundingClientRect().height || 0;
      const availW = Math.max(1, window.innerWidth);
      const availH = Math.max(1, window.innerHeight - statusH - controlsH);
      const scale = Math.max(1, Math.min(availW / W, availH / H));
      canvas.style.width = (W * scale) + 'px';
      canvas.style.height = (H * scale) + 'px';
    }

    window.addEventListener('resize', resizeCanvas);
    resizeCanvas();

    // 防止拖曳/選取（iOS）
    ['touchstart','touchmove','touchend','touchcancel','dragstart'].forEach(evt => {
      canvas.addEventListener(evt, e => e.preventDefault(), { passive: false });
    });

    // 阻止雙擊縮放與手勢縮放
    ['dblclick','gesturestart'].forEach(evt => {
      window.addEventListener(evt, e => e.preventDefault(), { passive: false });
    });

    // 阻止全局捲動與捏合導致視窗裁切
    ['touchmove','gesturechange'].forEach(evt => {
      window.addEventListener(evt, e => e.preventDefault(), { passive: false });
    });

    // 按鍵事件上報：key=按鍵名稱, type=down/up/cancel，避免漏掉 up
    function sendInput(key, type) {
      fetch(`/input?key=${encodeURIComponent(key)}&type=${encodeURIComponent(type)}` , {
        method: 'POST',
        keepalive: true
      }).catch(()=>{});
    }

    function bindButton(el){
      const key = el.dataset.key;
      if (!key) return;
      const activePointers = new Set();

      const endPress = (pointerId, kind='up') => {
        if (!activePointers.has(pointerId)) return;
        activePointers.delete(pointerId);
        sendInput(key, kind);
      };

      el.addEventListener('pointerdown', e => {
        e.preventDefault();
        activePointers.add(e.pointerId);
        sendInput(key, 'down');
        try { el.setPointerCapture(e.pointerId); } catch (_) {}
      });

      el.addEventListener('pointerup', e => {
        e.preventDefault();
        endPress(e.pointerId, 'up');
        try { el.releasePointerCapture(e.pointerId); } catch (_) {}
      });

      el.addEventListener('pointercancel', e => {
        e.preventDefault();
        endPress(e.pointerId, 'cancel');
      });

      el.addEventListener('pointerleave', e => {
        // 若未捕捉到 up 且手指滑出元件，視為 cancel 防止卡鍵
        endPress(e.pointerId, 'cancel');
      });
    }

    document.querySelectorAll('.btn[data-key], .meta-btn[data-key]').forEach(bindButton);

    let currentPaletteVer = -1;

    async function fetchFrame(){
      try{
        const res=await fetch('/frame', {cache:'no-store'});
        if(!res.ok){ statusEl.textContent='等待畫面...'; setTimeout(fetchFrame, 16); return; }
        
        // Check palette version
        const paletteVer = parseInt(res.headers.get('X-Palette-Ver') || '0');
        const hasPalette = (paletteVer !== currentPaletteVer);
        if (hasPalette) currentPaletteVer = paletteVer;
        
        const buf=new Uint8Array(await res.arrayBuffer());
        let pixels;
        
        if (hasPalette) {
          // Frame with palette
          if(buf.length < 256*3 + W*H){ statusEl.textContent='畫面不完整'; setTimeout(fetchFrame, 16); return; }
          palette.set(buf.subarray(0, 256*3));
          pixels = buf.subarray(256*3);
        } else {
          // Frame only
          if(buf.length < W*H){ statusEl.textContent='畫面不完整'; setTimeout(fetchFrame, 16); return; }
          pixels = buf;
        }
        
        for(let i=0,j=0;i<pixels.length;i++){
          const idx=pixels[i]*3;
          img.data[j++]=palette[idx];
          img.data[j++]=palette[idx+1];
          img.data[j++]=palette[idx+2];
          img.data[j++]=255;
        }
        ctx.putImageData(img,0,0);
        
        // 更新 FPS 計數
        frameCount++;
        const now = performance.now();
        const elapsed = now - lastFpsUpdate;
        
        // 每 5 秒更新一次 FPS 顯示
        if (elapsed >= 5000) {
          currentFps = Math.round((frameCount / elapsed) * 1000);
          frameCount = 0;
          lastFpsUpdate = now;
        }
        
        statusEl.textContent = currentFps > 0 ? `串流中 @ ${currentFps} fps` : '串流中...';
      }catch(e){
        statusEl.textContent='錯誤: '+e;
      }
      // 立即發起下一個請求，不等 rAF
      setTimeout(fetchFrame, 0);
    }
    fetchFrame();
  </script>
</body>
</html>
)rawliteral";

// ===== HTTP Handlers =====

static void handleRoot()
{
    // Ensure palette is resent after a fresh page load/reconnect
    palette_needs_update = 1;

    client_connected = 1; // Mark client as connected

    // Try to read index.html from SD card
    FILE *file = fopen("/sdcard/index.html", "r");
    if (file != NULL)
    {
        // Seek to end to get file size
        fseek(file, 0, SEEK_END);
        long filesize = ftell(file);
        fseek(file, 0, SEEK_SET);

        // Allocate buffer and read file
        char *buffer = (char *)malloc(filesize + 1);
        if (buffer != NULL)
        {
            size_t read_size = fread(buffer, 1, filesize, file);
            buffer[read_size] = '\0';

            webServer.send(200, "text/html", buffer);
            free(buffer);
            fclose(file);
            return;
        }
        fclose(file);
    }

    // Fallback to embedded HTML if file not found
    webServer.send_P(200, "text/html", FALLBACK_HTML);
}

static void handleFrame()
{
    client_connected = 1; // Mark client as connected

    // Snapshot the current read buffer index and stable palette under fb_mutex.
    // Sets fb_read_locked=1 so Core 0 will not overwrite this buffer during TCP write.
    uint8_t local_pal[256 * 3];
    int snap_idx = DRIVER_Web_BeginRead(local_pal);

    const uint8_t *frame = DRIVER_Web_GetFrameBuffer(snap_idx);
    if (frame == NULL)
    {
        DRIVER_Web_EndRead();
        webServer.send(503, "text/plain", "Frame not ready");
        return;
    }

    // Determine if palette should be included in this response
    int send_palette_flag = 0;
    if (palette_needs_update)
    {
        palette_needs_update = 0;
        palette_version++;
        send_palette_flag = 1;
    }

    webServer.sendHeader("X-Palette-Ver", String(palette_version));

    DRIVER_TransmitVideoFrame(frame, send_palette_flag ? local_pal : NULL);

    // Release read lock so Core 0 can swap buffers on next frame
    DRIVER_Web_EndRead();
}

static void handleNotFound()
{
    webServer.sendHeader("Location", String("http://") + AP_IP.toString(), true);
    webServer.send(302, "text/plain", "Redirect");
}

static void handleInput()
{
    client_connected = 1; // Mark client as connected

    String key = webServer.arg("key");
    String type = webServer.arg("type");

    if (key.length() == 0)
    {
        webServer.send(400, "text/plain", "missing key");
        return;
    }

    // Push to input queue; game loop on Core 0 will drain it via DRIVER_Process_Events()
    DRIVER_WebInputQueue_Push(key.c_str(), type.c_str());

    webServer.send(200, "text/plain", "ok");
}

// ===== Public API =====

// frame: pointer to the snapshotted framebuffer (back_buffer[snap_idx])
// palette: pointer to local palette copy, or NULL if no palette update this frame
void DRIVER_TransmitVideoFrame(const uint8_t *frame, const uint8_t *palette)
{
    WiFiClient client = webServer.client();

    if (palette != NULL)
    {
        const size_t payloadSize = (256 * 3) + 320 * 200;
        webServer.setContentLength(payloadSize);
        webServer.send(200, "application/octet-stream", "");
        client.write(palette, 256 * 3);
        client.write(frame, 320 * 200);
    }
    else
    {
        const size_t payloadSize = 320 * 200;
        webServer.setContentLength(payloadSize);
        webServer.send(200, "application/octet-stream", "");
        client.write(frame, 320 * 200);
    }
    client.flush();
}

void DRIVER_MarkPaletteUpdate(void)
{
    palette_needs_update = 1;
}

int DRIVER_InitWebServer(void)
{
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(AP_IP, AP_IP, AP_NETMASK);
    WiFi.softAP(AP_SSID, ""); // open hotspot

    dnsServer.start(DNS_PORT, "*", AP_IP);

    webServer.on("/", HTTP_GET, handleRoot);
    webServer.on("/frame", HTTP_GET, handleFrame);
    webServer.on("/input", HTTP_ANY, handleInput);
    webServer.onNotFound(handleNotFound);
    webServer.begin();

    Serial.println("========================================");
    Serial.println("Waiting for web client connection...");
    Serial.println("Connect your device to 'PAL-AP' hotspot");
    Serial.println("then open a web browser to access the game");
    Serial.println("========================================");

    while (!client_connected)
    {
        // Process web server requests to allow client to connect
        DRIVER_ProcessWebServer();

        // Small delay to avoid busy-waiting
        delay(100);
    }

    Serial.println("Client connected! Starting game...");

    return 0;
}

void DRIVER_DeInitWebServer(void)
{
    webServer.stop();
    dnsServer.stop();
    WiFi.mode(WIFI_OFF);
}

void DRIVER_ProcessWebServer(void)
{
    dnsServer.processNextRequest();
    webServer.handleClient();
}
#endif /* ARDUINO_ARCH_ESP32 && PAL_ESP32_BACKEND_WEB */
