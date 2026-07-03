#ifndef PAL_ESP32_BACKEND_SELECT_H
#define PAL_ESP32_BACKEND_SELECT_H

/* Selects exactly ONE ESP32 backend for the Arduino build.
   Two mutually-exclusive backends live under src/driver_esp32/:
     - web/ : WiFi web-streaming backend (frame streamed to a browser canvas).
     - hw/  : hardware backend (ILI9341 TFT / NTSC composite video + Bluepad32 gamepad).

   Note: arduino-cli compiles every source under src/ recursively, so both backends
   are always seen by the compiler. Each backend .c/.cpp is wrapped in
   `#if defined(ARDUINO_ARCH_ESP32) && PAL_ESP32_BACKEND_xxx`, so only the selected
   one produces code and the DRIVER_* symbols are never defined twice.

   IDE users edit the two defaults below. arduino-cli users override them via a build
   property, e.g. -DPAL_ESP32_BACKEND_WEB=1 -DPAL_ESP32_BACKEND_HW=0 (the #ifndef guards
   let the -D override win over the defaults). */

/* Default to the web backend when no -D override is supplied. */
#ifndef PAL_ESP32_BACKEND_WEB
#define PAL_ESP32_BACKEND_WEB 1
#endif
#ifndef PAL_ESP32_BACKEND_HW
#define PAL_ESP32_BACKEND_HW  0
#endif

/* Reject a build that selects zero or both backends. */
#if (PAL_ESP32_BACKEND_WEB + PAL_ESP32_BACKEND_HW) != 1
#error "Select exactly one ESP32 backend: set PAL_ESP32_BACKEND_WEB or PAL_ESP32_BACKEND_HW to 1 (not both)."
#endif

#endif /* PAL_ESP32_BACKEND_SELECT_H */
