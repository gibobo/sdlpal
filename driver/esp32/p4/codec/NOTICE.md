ES8311 driver copied from Waveshare's ESP32-P4-WIFI6-Touch-LCD-3.5 examples:
https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-3.5/tree/870588e62daaf723f3c52f0060d8dc053665ed30/examples/arduino/libraries/Waveshare_LCD35/src

Original copyright: Espressif Systems, Apache-2.0 (see LICENSE).
Local modifications: guard es8311.c with the PAL_ESP32_BACKEND_P4 build selector
so Arduino's recursive source compilation leaves other backends unaffected;
add an explicitly ranged half-dB DAC gain API for calibrated volume control.

The suspend register sequence follows Espressif's Apache-2.0 codec driver,
Copyright 2023 Espressif Systems (Shanghai) CO LTD:
https://github.com/espressif/esp-adf/blob/release/v2.x/components/esp_codec_dev/device/es8311/es8311.c
Local code adds register readback and uses full initialization when resuming.
