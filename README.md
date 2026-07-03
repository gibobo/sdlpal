# sdlpal

《仙劍奇俠傳》(Pal) 遊戲引擎的 SDLPAL 衍生版本，**單一 repo 同時支援桌面與 ESP32 兩種平台**：

- **桌面**：以 CMake 建置，多種後端（SDL / GLFW / 終端機 CACA / Web / Dummy）。
- **ESP32**：以 Arduino CLI 建置，兩種可於 build 時切換的後端：
  - **web** — ESP32 開 WiFi AP，畫面串流到瀏覽器 canvas，輸入從網頁進來（偏向 ESP32-S3）。
  - **hw** — 直接驅動 ILI9341 TFT（S3）或 NTSC 複合視訊（classic ESP32/WROVER）+ Bluepad32 手柄，獨立掌上機型。

可攜的遊戲引擎核心只有一份（`src/`），桌面與 ESP32 共用；平台差異全部藏在 `#ifdef` guard 與各自的後端驅動層裡。

---

## 專案結構

```
sdlpal/
├─ sdlpal.ino                 # ESP32 Arduino sketch 入口（檔名須等於資料夾名）
├─ CMakeLists.txt             # 桌面建置（CMake project 名為 palgame）
├─ sketch.yaml                # (可選) arduino-cli build profile
├─ decode-backtrace.ps1       # ESP32 當機 Backtrace 解碼工具
├─ .vscode/
│  ├─ tasks.json              # 每後端 Compile / Compile+Upload / Monitor / Board list
│  └─ settings.json.sample    # 複製為 settings.json，填入序列埠與 FQBN
├─ src/                       # 可攜遊戲引擎核心（桌面 CMake 與 ESP32 皆編譯）
│  ├─ *.c *.h  adplug/  sound/
│  ├─ pcmmus.c / pcmmus.h     # 預渲染 PCM 音樂播放器（音樂快取，見下）
│  ├─ driver.h                # 後端共用的 HAL 介面（DRIVER_*）
│  └─ driver_esp32/           # ESP32 後端（只有 arduino-cli 會編）
│     ├─ backend_select.h     # 後端選擇器（exactly one）
│     ├─ web/                 # WiFi 串流後端 + utils/（webserver、pins、index.html）
│     └─ hw/                  # TFT/複合視訊/手柄後端 + utils/（ili9341、composite、bluepad32、pins）
├─ tools/musrender.c          # 離線音樂預渲染工具（palmusrender target，見下）
├─ driver/                    # 桌面後端（SDL/GLFW/CACA/WEB/Dummy）— ESP32 忽略
├─ external/                  # git submodules：SDL / glfw / libcaca / miniaudio / mongoose
└─ shaders/                   # 桌面 GLSL 後處理 shader
```

> **為什麼 ESP32 後端放在 `src/driver_esp32/`？** Arduino CLI 只會遞迴編譯 sketch 的 `src/` 底下的檔案，因此 ESP32 驅動必須放在 `src/` 下。桌面 CMake 的 glob 是非遞迴的，看不到 `src/driver_esp32/`，兩套建置系統因此在同一份樹上共存。

---

## 桌面建置（CMake）

**前置需求**：CMake ≥ 3.14、C/C++ 編譯器（MSVC / GCC / Clang）、OpenGL、（可選）zlib。

```bash
# 1. 取得 submodules（SDL / glfw / libcaca / miniaudio / mongoose）
git submodule update --init --recursive

# 2. Configure + build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

會產生 5 個遊戲執行檔（於 `build/`），外加離線工具 `palmusrender`（見 [預渲染 PCM 音樂](#預渲染-pcm-音樂音樂快取)）：

| Target | 後端 |
|---|---|
| `palgame_sdl` | SDL2（一般桌面推薦）|
| `palgame_glfw` | GLFW + OpenGL |
| `palgame_caca` | libcaca（終端機文字畫面）|
| `palgame_dummy` | 無畫面（測試用）|
| `palgame_webserver` | mongoose HTTP（含 emscripten/WASM 路徑）|

> 執行時需要《仙劍》遊戲資料檔（放在執行檔旁的 `resource/` 目錄，即 `RESOURCE_PATH`）。VSCode 使用者可用 *CMake Tools* 擴充直接 configure/build/run。

---

## ESP32 建置（Arduino CLI）

**前置需求**

1. 安裝 [arduino-cli](https://arduino.github.io/arduino-cli/)。
2. 安裝 ESP32 core：
   - `esp32:esp32`（web 後端使用）
   - `esp32-bluepad32:esp32`（hw 後端需要，提供 Bluepad32 手柄堆疊）——需先加入其 board manager URL：
     `https://raw.githubusercontent.com/ricardoquesada/esp32-arduino-lib-builder/master/bluepad32_files/package_esp32_bluepad32_index.json`
   ```bash
   arduino-cli core install esp32:esp32
   arduino-cli core install esp32-bluepad32:esp32
   ```
   WiFi / WebServer / DNSServer / SPI 皆由 core 內建，無需另裝 library。

> ⚠️ **資料夾名必須等於 sketch 名**：本 repo 的資料夾要叫 `sdlpal`（對應 `sdlpal.ino`），否則 arduino-cli 會找不到主 sketch。

### 後端選擇（build 時）

由 `src/driver_esp32/backend_select.h` 決定，預設為 **web**。切換方式二選一：

- **改預設**（IDE 直接編譯用）：編輯 `backend_select.h` 的兩個 `#define`。
- **build 時覆寫**（推薦，VSCode task 已內建）：傳 `-DPAL_ESP32_BACKEND_WEB` / `-DPAL_ESP32_BACKEND_HW`。

### 用 VSCode（推薦，取代 Arduino IDE）

1. 複製 `.vscode/settings.json.sample` → `.vscode/settings.json`，填入：
   - `esp32.port`：序列埠（用 **ESP: Board list** task 查）
   - `esp32.webFqbn`：web 後端的 board FQBN（預設 `esp32:esp32:esp32s3`）
   - `esp32.hwFqbn`：hw 後端的 board FQBN（`esp32-bluepad32:esp32:esp32wrover`）
2. 執行對應 task（`Ctrl+Shift+B` 或 Run Task）：
   - **ESP web: Compile** / **ESP web: Compile+Upload**
   - **ESP hw: Compile** / **ESP hw: Compile+Upload**
   - **ESP: Monitor**（序列埠監看，115200）

### 用命令列

```bash
# web 後端（ESP32-S3）
arduino-cli compile --fqbn esp32:esp32:esp32s3 \
  --build-property "compiler.c.extra_flags=-DPAL_ESP32_BACKEND_WEB=1 -DPAL_ESP32_BACKEND_HW=0" \
  --build-property "compiler.cpp.extra_flags=-DPAL_ESP32_BACKEND_WEB=1 -DPAL_ESP32_BACKEND_HW=0" \
  --build-path ./build_arduino .

# hw 後端（classic ESP32 / WROVER）— 注意 -DOPL3_IRAM_ATTR= 見下方說明
arduino-cli compile --fqbn esp32-bluepad32:esp32:esp32wrover \
  --build-property "compiler.c.extra_flags=-DPAL_ESP32_BACKEND_WEB=0 -DPAL_ESP32_BACKEND_HW=1 -DOPL3_IRAM_ATTR=" \
  --build-property "compiler.cpp.extra_flags=-DPAL_ESP32_BACKEND_WEB=0 -DPAL_ESP32_BACKEND_HW=1 -DOPL3_IRAM_ATTR=" \
  --build-path ./build_arduino .
```

### 執行需求

- **遊戲資料**：放在 SD 卡根目錄（掛載於 `/sdcard`，即 ESP32 上的 `RESOURCE_PATH`）。
- **（強烈建議）預渲染音樂**：把 `<song>_22050.pcm` 放到 SD 卡 `/sdcard/mus/`，即可免去 ESP32 上即時 OPL3 FM 合成的沉重運算。ESP32 端此功能**預設開啟**；產生方式見 [預渲染 PCM 音樂](#預渲染-pcm-音樂音樂快取)。
- **web 後端**：另需把 `src/driver_esp32/web/index.html` 放到 SD 卡 `/sdcard/index.html`；開機後連上 WiFi AP **`PAL-AP`**，瀏覽器開 `http://4.3.2.1` 即可看到畫面並操作。
- **hw 後端**：接好 ILI9341 TFT（S3）或複合視訊輸出（classic ESP32），並配對 Bluepad32 支援的藍牙手柄。腳位定義見各後端的 `utils/esp32_pins.h`。

### ⚠️ hw 後端在 classic ESP32 的 IRAM 限制

引擎的 OPL3 音源在 `ESP_PLATFORM` 下會被放進 IRAM 以提升效能。在 **classic ESP32 (WROVER)** 上，這與 Bluepad32/BTstack 一起會使 IRAM 溢位（`.iram0.text overflowed`）。因此 hw 後端需以 `-DOPL3_IRAM_ATTR=` 把 OPL3 放回 flash（VSCode 的 hw task 與 `decode-backtrace.ps1` 已內建此旗標）。web/S3 後端 IRAM 有餘裕，維持 IRAM 放置。

### 當機除錯

貼上序列埠印出的 `Backtrace:` 行給解碼器即可定位：

```powershell
.\decode-backtrace.ps1              # web 後端（預設）
.\decode-backtrace.ps1 -Backend hw  # hw 後端
```

---

## 預渲染 PCM 音樂（音樂快取）

《仙劍》的背景音樂是 RIX 格式，靠 **Nuked-OPL3 FM 合成**即時產生 —— 這在桌面沒問題，但在 ESP32 上是最吃 CPU 的工作。本功能把合成「離線做一次」：在桌面用引擎**原封不動的同一套 OPL3/RIX 程式碼**把每首曲子渲染成 PCM 存檔，執行期改成單純讀檔播放，ESP32 因此不必再跑即時合成。

分成兩個元件：

| 元件 | 位置 | 作用 |
|---|---|---|
| **`palmusrender`**（離線工具）| [tools/musrender.c](tools/musrender.c) | 桌面 CMake target。用引擎的 Nuked-OPL3/RIX 合成每首曲子一次（輸出與遊戲即時播放**位元相同**），再用引擎 resampler 轉成任意取樣率，寫出帶標頭的 PCM 檔。 |
| **`PCMMUS`**（執行期播放器）| [src/pcmmus.c](src/pcmmus.c) | `AUDIOPLAYER` 後端，直接串流 `<RESOURCE_PATH>/mus/<song>_<rate>.pcm`。找不到預渲染檔時**自動退回** RIX 即時合成。 |

### 開關：`PAL_PRERENDERED_MUSIC`

音樂後端由 [src/audio.h](src/audio.h) 的 `PAL_PRERENDERED_MUSIC` 選擇：

- **ESP32**：預設 **1（開）** —— 有 `mus/` 檔就用快取，否則退回 RIX。
- **桌面**：預設 **0（關，走 RIX）**。以 CMake 選項開啟，用於**驗證／對照**：
  ```bash
  cmake -S . -B build -DPAL_PRERENDERED_MUSIC=ON
  cmake --build build --target palgame_sdl
  ```

### 產生 PCM 檔

```bash
# 1. 建置離線工具
cmake --build build --target palmusrender

# 2. 渲染（--rate 可重複；預設同時產 22050 與 44100）
build/palmusrender --data <遊戲資料夾> --out ./mus_pcm --rate 22050 --rate 44100
#   --data     含 MUS.MKF 的遊戲資料夾（例：Pal98rqptw）
#   --out      輸出資料夾（自動建立）
#   --rate     目標取樣率(Hz)，可重複；不綁死於 22050/44100
#   --quality  resampler 品質 0..4（預設 4 = SINC）
```

### 檔案格式與命名

- 命名：`<song>_<rate>.pcm`（例：`5_22050.pcm`）。
- 內容：12-byte 小端標頭（magic `PCM1` + version + channels + sample_rate）後接**交錯立體聲 int16 LE** 原始取樣，與引擎混音緩衝區佈局相同，因此執行期播放只是「讀檔→直接送輸出」。

### 佈署（取樣率須對應目標平台）

平台的輸出取樣率固定於 [src/audio.h](src/audio.h)：**桌面 44100 Hz、ESP32 22050 Hz**。放入對應取樣率的檔案即可：

| 平台 | 放置位置 | 取樣率 |
|---|---|---|
| 桌面 | `<build>/resource/mus/`（即 `RESOURCE_PATH/mus/`）| `*_44100.pcm` |
| ESP32 | SD 卡 `/sdcard/mus/` | `*_22050.pcm` |

> 因為 44100 檔是**未經重取樣的 OPL3 原始輸出**，桌面開啟 `PAL_PRERENDERED_MUSIC` 後聽感應與 RIX 版**完全相同** —— 這正是驗證整條快取鏈路正確性的最佳對照。

---

## 開發流程

- 引擎核心的修改**只在 `src/` 進行**，桌面與 ESP32 兩平台自動共享，不需再手動同步。
- 切換平台＝選擇要跑哪個建置工具：桌面走 CMake、ESP32 走 arduino-cli，全部可在同一個 VSCode workspace 內完成，不必開 Arduino IDE。

---

## 授權

本專案衍生自 [SDLPAL](https://github.com/sdlpal/sdlpal)，採 **GPLv3** 授權（見 `LICENSE`）。各檔案標頭保留原始版權宣告。

## 致謝

- [SDLPAL 開發團隊](https://github.com/sdlpal/sdlpal) — 引擎本體。
- ESP32 web / 硬體後端源自 `esp32-pal` 專案，已整併至本 repo。
- [Bluepad32](https://github.com/ricardoquesada/bluepad32)（Ricardo Quesada）— 藍牙手柄支援。
- NTSC 複合視訊輸出改自 Peter Barrett / bitluni 的 ESP32 composite video 實作。
