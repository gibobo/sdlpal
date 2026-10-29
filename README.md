# sdlpal

《仙劍奇俠傳》(Pal) 遊戲引擎的 SDLPAL 衍生版本，**單一 repo 同時支援桌面與 ESP32 兩種平台**：

- **桌面**：以 CMake 建置，多種後端（SDL / GLFW / 終端機 CACA / Web / Dummy）。
- **ESP32**：以 Arduino CLI 建置，三種可於 build 時切換的後端：
  - **web** — ESP32 開 WiFi AP，畫面串流到瀏覽器 canvas，輸入從網頁進來（偏向 ESP32-S3）。
  - **p4** — Waveshare ESP32-P4 3.5 吋螢幕、觸控、音訊與 SD 卡，詳見 [P4 說明](tools/ESP32-P4.md)。
  - **hw** — 直接驅動 ILI9341 TFT（S3）或 NTSC 複合視訊（classic ESP32/WROVER）+ Bluepad32 手柄，獨立掌上機型。

可攜的遊戲引擎核心只有一份（`src/`），桌面與 ESP32 共用；平台差異全部藏在 `#ifdef` guard 與各自的後端驅動層裡。

---

## 專案結構

```text
sdlpal/
├─ sdlpal.ino                 # Arduino 入口，由建置腳本準備暫存 sketch
├─ CMakeLists.txt             # 桌面建置
├─ src/                       # 各平台共用的遊戲核心、HAL 介面與工具
├─ driver/                    # 統一的各平台驅動目錄
│  ├─ *_SDL.c、*_GLFW.c…      # 既有桌面後端
│  ├─ utils/                  # 桌面驅動工具
│  └─ esp32/
│     ├─ backend_select.h     # web / hw / p4 選擇器
│     ├─ web/                 # WiFi 串流
│     ├─ hw/                  # TFT／複合視訊／手柄
│     └─ p4/                  # Waveshare LCD35 顯示、觸控、音訊與 SD
├─ tools/
│  ├─ build-esp32.ps1         # 共用 Arduino 暫存建置流程
│  ├─ build-p4.ps1            # P4 相容入口
│  ├─ ESP32-P4.md             # P4 操作與驗證紀錄
│  └─ musrender.c             # 離線音樂預渲染
├─ external/                  # SDL / glfw / libcaca / miniaudio / mongoose
└─ shaders/                   # 桌面 shader
```

> **Arduino 如何編譯外層驅動？** `tools/build-esp32.ps1` 將共用 `src/` 與 `driver/esp32/` 複製到 `build/arduino-stage/<backend>/sdlpal/src/`，並只在暫存 sketch 中調整相對 include 路徑。原始碼維持單一份，桌面驅動不會進入 Arduino 編譯。請使用建置腳本或 VSCode task。

P4 的平台策略集中在 `driver/esp32/p4/`：

- `board.cpp`：顯示、觸控、音訊、音量與 SD 硬體。
- `landscape.cpp/.h`：480 × 300 等比例顯示、局部更新與控制圖層合成。
- `controls.cpp/.h`：六區隱藏按鍵、R/A、F/D 與確認/AUTO 短長按、固定方向控制與單一箭頭提示、0–5 音量（預設 1）及長壓工具列手勢。
- `timing.c`：將繪圖時間計入每幀預算、淡入淡出的呈現排程與診斷。
- `platform.c`：關機順序、重新啟動與 SD 卡既有 PCM 路徑相容。
- `platform.h`：上述模組的介面。
- `power_state.h`：PMIC 電源鍵事件判斷與睡眠期間的遊戲時計處理；睡眠硬體協調位於 `board.cpp`。

`src/util.c` 是 P4 與共用核心的唯一條件分派入口。走路、戰鬥與轉場仍需在核心保留通用的計時呼叫；走路、戰鬥與轉場的程式不再直接引用 P4 巨集、ESP SDK 或 P4 日誌。其他平台沿用原本的延遲與關機流程。戰鬥淡出時重畫「圍攻」文字屬於共用的畫面修正，保留於核心。

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

> 建置腳本會建立名為 `sdlpal` 的暫存 sketch，因此專案資料夾可保留目前名稱。P4 另需 GFX Library for Arduino，版本與安裝位置見 [P4 說明](tools/ESP32-P4.md)。

### 後端選擇（build 時）

由 `driver/esp32/backend_select.h` 提供互斥選擇器；建置腳本透過 `-Backend web`、`-Backend hw` 或 `-Backend p4` 明確指定，無需手動編輯巨集。

### 用 VSCode（推薦，取代 Arduino IDE）

1. 複製 `.vscode/settings.json.sample` → `.vscode/settings.json`，填入：
   - `esp32.port`：序列埠（用 **ESP: Board list** task 查）
   - `esp32.webFqbn`：web 後端的 board FQBN（預設 `esp32:esp32:esp32s3`）
   - `esp32.hwFqbn`：hw 後端的 board FQBN（`esp32-bluepad32:esp32:esp32wrover`）
2. 執行對應 task（`Ctrl+Shift+B` 或 Run Task）：
   - **ESP web: Compile** / **ESP web: Compile+Upload**
   - **ESP hw: Compile** / **ESP hw: Compile+Upload**
   - **ESP p4: Compile** / **ESP p4: Compile+Upload**
   - **ESP: Monitor**（序列埠監看，115200）

### 用命令列

```powershell
# web（ESP32-S3）
./tools/build-esp32.ps1 -Backend web

# hw（classic ESP32 / WROVER + Bluepad32）
./tools/build-esp32.ps1 -Backend hw

# P4；加入 -Upload 燒錄，-Port 指定連接埠
./tools/build-p4.ps1
./tools/build-p4.ps1 -Upload -Port COM3

# 可用 -Fqbn 覆寫其他板型
./tools/build-esp32.ps1 -Backend web -Fqbn esp32:esp32:esp32s3
```

### 執行需求

- **遊戲資料**：放在 SD 卡根目錄（掛載於 `/sdcard`，即 ESP32 上的 `RESOURCE_PATH`）。
- **（強烈建議）預渲染音樂**：把 `<song>_22050.pcm` 放到 SD 卡 `/sdcard/mus/`，即可免去 ESP32 上即時 OPL3 FM 合成的沉重運算。ESP32 端此功能**預設開啟**；產生方式見 [預渲染 PCM 音樂](#預渲染-pcm-音樂音樂快取)。
- **web 後端**：另需把 `driver/esp32/web/index.html` 放到 SD 卡 `/sdcard/index.html`；開機後連上 WiFi AP **`PAL-AP`**，瀏覽器開 `http://4.3.2.1` 即可看到畫面並操作。
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
