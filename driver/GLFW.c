#include "driver.h"

int DRIVER_Init(void) {

  return 0;
}

void DRIVER_DeInit(void) {
  DRIVER_DeInit_Audio();
  DRIVER_DeInit_Video();
  DRIVER_DeInit_Event();
}
