#include "driver.h"
#include "src/input.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern GLFWwindow *window;
static unsigned char *rgdwKeyLastTime = NULL;
static unsigned char rgdwKeyCount = 0;
static unsigned int counter = 0; // Event index
static const int g_KeyMap[][2] = {
    {GLFW_KEY_UP, kKeyUp},
    {GLFW_KEY_KP_8, kKeyUp},
    {GLFW_KEY_DOWN, kKeyDown},
    {GLFW_KEY_KP_2, kKeyDown},
    {GLFW_KEY_LEFT, kKeyLeft},
    {GLFW_KEY_KP_4, kKeyLeft},
    {GLFW_KEY_RIGHT, kKeyRight},
    {GLFW_KEY_KP_6, kKeyRight},
    {GLFW_KEY_ESCAPE, kKeyMenu},
    {GLFW_KEY_INSERT, kKeyMenu},
    {GLFW_KEY_LEFT_ALT, kKeyMenu},
    {GLFW_KEY_RIGHT_ALT, kKeyMenu},
    {GLFW_KEY_KP_0, kKeyMenu},
    {GLFW_KEY_ENTER, kKeySearch},
    {GLFW_KEY_SPACE, kKeySearch},
    {GLFW_KEY_KP_ENTER, kKeySearch},
    {GLFW_KEY_LEFT_CONTROL, kKeySearch},
    {GLFW_KEY_PAGE_UP, kKeyPgUp},
    {GLFW_KEY_KP_9, kKeyPgUp},
    {GLFW_KEY_PAGE_DOWN, kKeyPgDn},
    {GLFW_KEY_KP_3, kKeyPgDn},
    {GLFW_KEY_HOME, kKeyHome},
    {GLFW_KEY_KP_7, kKeyHome},
    {GLFW_KEY_END, kKeyEnd},
    {GLFW_KEY_KP_1, kKeyEnd},
    {GLFW_KEY_R, kKeyRepeat},
    {GLFW_KEY_A, kKeyAuto},
    {GLFW_KEY_D, kKeyDefend},
    {GLFW_KEY_E, kKeyUseItem},
    {GLFW_KEY_W, kKeyThrowItem},
    {GLFW_KEY_Q, kKeyFlee},
    {GLFW_KEY_F, kKeyForce},
    {GLFW_KEY_S, kKeyStatus}};

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
   unsigned char i;
   for (i = 0; i < rgdwKeyCount; i++)
   {
      if (g_KeyMap[i][0] == key)
      {
         if (action != GLFW_RELEASE)
         {
            PAL_KeyDown(g_KeyMap[i][1], (rgdwKeyLastTime[i] != 0));
            rgdwKeyLastTime[i] = 0xFF;
         }
         else
         {
            PAL_KeyUp(g_KeyMap[i][1]);
            rgdwKeyLastTime[i] = 0;
         }
      }
   }
}

static void joystick_callback(int jid, int event)
{
   if (event == GLFW_CONNECTED) {
      int axisCount, buttonCount, hatCount;

      glfwGetJoystickAxes(jid, &axisCount);
      glfwGetJoystickButtons(jid, &buttonCount);
      glfwGetJoystickHats(jid, &hatCount);

      printf("%08x at %0.3f: Joystick %i (%s) was connected with %i axes, %i buttons, and %i hats\n",
            counter++, glfwGetTime(),
            jid,
            glfwGetJoystickName(jid),
            axisCount,
            buttonCount,
            hatCount);

      if (glfwJoystickIsGamepad(jid)) {
         printf("  Joystick %i (%s) has a gamepad mapping (%s)\n",
               jid,
               glfwGetJoystickGUID(jid),
               glfwGetGamepadName(jid));
      } else {
         printf("  Joystick %i (%s) has no gamepad mapping\n",
               jid,
               glfwGetJoystickGUID(jid));
      }
   } else {
      printf("%08x at %0.3f: Joystick %i was disconnected\n",
            counter++, glfwGetTime(), jid);
   }
}

int DRIVER_Process_Events(void) {
  int res = 0;
    /* Poll for and process events */
  if (glfwWindowShouldClose(window))
    res = -1;
  else
    glfwPollEvents();
  return res;
}

int DRIVER_Init_Event(void) {
   glfwSetKeyCallback(window, key_callback);
   glfwSetJoystickCallback(joystick_callback);
   rgdwKeyCount = sizeof(g_KeyMap) / sizeof(g_KeyMap[0]);
   rgdwKeyLastTime = (unsigned char *)calloc(rgdwKeyCount, sizeof(unsigned char));
   return 0;
}

void DRIVER_DeInit_Event(void) {
}
