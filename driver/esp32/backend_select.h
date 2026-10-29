#ifndef PAL_ESP32_BACKEND_SELECT_H
#define PAL_ESP32_BACKEND_SELECT_H

/* Selects exactly ONE ESP32 backend for the Arduino build.
   Three mutually-exclusive backends live under driver/esp32/:
     - web/ : WiFi web-streaming backend (frame streamed to a browser canvas).
     - hw/  : hardware backend (ILI9341 TFT / NTSC composite video + Bluepad32 gamepad).
     - p4/  : Waveshare LCD35 display, touch, audio and SD support.

   The build script stages this directory under the sketch's src/ so all backends
   are always seen by the compiler. Each backend .c/.cpp is wrapped in
   `#if defined(ARDUINO_ARCH_ESP32) && PAL_ESP32_BACKEND_xxx`, so only the selected
   one produces code and the DRIVER_* symbols are never defined twice.

   The build script overrides the defaults below via a build
   property, e.g. -DPAL_ESP32_BACKEND_WEB=1 -DPAL_ESP32_BACKEND_HW=0 (the #ifndef guards
   let the -D override win over the defaults). */

/* Default to the web backend when no -D override is supplied. */
#ifndef PAL_ESP32_BACKEND_WEB
#define PAL_ESP32_BACKEND_WEB 1
#endif
#ifndef PAL_ESP32_BACKEND_HW
#define PAL_ESP32_BACKEND_HW  0
#endif
#ifndef PAL_ESP32_BACKEND_P4
#define PAL_ESP32_BACKEND_P4 0
#endif

/* Reject a build that selects zero or multiple backends. */
#if (PAL_ESP32_BACKEND_WEB + PAL_ESP32_BACKEND_HW + PAL_ESP32_BACKEND_P4) != 1
#error "Select exactly one ESP32 backend: WEB, HW, or P4."
#endif

#endif /* PAL_ESP32_BACKEND_SELECT_H */
