#include "../backend_select.h"
#if defined(ARDUINO_ARCH_ESP32) && PAL_ESP32_BACKEND_HW
#include "../../driver.h"
#include "../../input.h"
#include "DrvIf_internal.h"
#include "utils/bluepad32_api.h"
#include <Arduino.h>

// Button mapping table: Bluepad32 button index to PAL key
static const struct
{
    int bluepad32_button; // Bluepad32 button index
    PALKEY pal_key;       // PAL key constant
} g_GamepadKeyMap[] = {
    // D-pad mapping
    {BLUEPAD32_BUTTON_DPAD_UP, kKeyUp},
    {BLUEPAD32_BUTTON_DPAD_DOWN, kKeyDown},
    {BLUEPAD32_BUTTON_DPAD_LEFT, kKeyLeft},
    {BLUEPAD32_BUTTON_DPAD_RIGHT, kKeyRight},

    // Face buttons mapping
    {BLUEPAD32_BUTTON_A, kKeySearch}, // A = Search/Confirm
    {BLUEPAD32_BUTTON_B, kKeyMenu},   // B = Menu/Cancel
    {BLUEPAD32_BUTTON_X, kKeyStatus}, // X = Status
    {BLUEPAD32_BUTTON_Y, kKeyDefend}, // Y = Defend

    // Shoulder buttons
    {BLUEPAD32_BUTTON_L1, kKeyPgUp},      // L1 = Page Up
    {BLUEPAD32_BUTTON_R1, kKeyPgDn},      // R1 = Page Down
    {BLUEPAD32_BUTTON_L2, kKeyUseItem},   // L2 = Use Item
    {BLUEPAD32_BUTTON_R2, kKeyThrowItem}, // R2 = Throw Item

    // System buttons
    {BLUEPAD32_BUTTON_BACK, kKeyAuto},    // Back = Auto
    {BLUEPAD32_BUTTON_HOME, kKeyRepeat},  // Home = Repeat
    {BLUEPAD32_BUTTON_SYSTEM, kKeyForce}, // System = Force
};

#define GAMEPAD_KEY_MAP_SIZE (sizeof(g_GamepadKeyMap) / sizeof(g_GamepadKeyMap[0]))

// Button state tracking for edge detection
static unsigned char g_last_button_states[BLUEPAD32_BUTTON_COUNT];

// Analog stick state tracking
static int g_last_analog_x = 0;
static int g_last_analog_y = 0;

// Initialize event driver
int DRIVER_Init_Event(void)
{
    // Initialize button state tracking
    for (int i = 0; i < BLUEPAD32_BUTTON_COUNT; i++)
    {
        g_last_button_states[i] = 0;
    }

    // Initialize analog stick state
    g_last_analog_x = 0;
    g_last_analog_y = 0;

    // Initialize Bluepad32 API
    if (Bluepad32_Init() != 0)
    {
        return -1; // Initialization failed
    }

    // Start gamepad connection in background (non-blocking)
    // The game will continue to run even if gamepad is not connected
    printf("🎮 Starting gamepad connection in background...\n");
    printf("📱 Game will continue to run even without gamepad\n");

    return 0;
}

// Deinitialize event driver
void DRIVER_DeInit_Event(void)
{
    Bluepad32_Shutdown();
}

// Process input events
int DRIVER_Process_Events(void)
{
    // Update Bluepad32 controller states
    Bluepad32_Update();

    // Try to connect gamepad in non-blocking mode
    static unsigned long last_connect_attempt = 0;
    
    if (millis() - last_connect_attempt > 1000) { // Try every second
        int connect_result = Bluepad32_TryConnect();
        
        // Only print message when state changes
        if (connect_result == 1) {
            printf("🎮 Gamepad is now ready!\n");
        } else if (connect_result == -1) {
            printf("🎮 Gamepad disconnected, waiting for reconnection...\n");
        }
        // connect_result == 0 means no state change, don't print anything
        
        last_connect_attempt = millis();
    }

    // Handle gamepad disconnection (now non-blocking)
    Bluepad32_HandleReconnection();

    // Check if gamepad is ready
    if (!Bluepad32_IsGamepadReady())
    {
        return 0; // No controller ready, but don't block
    }

    // Process button events
    for (int i = 0; i < GAMEPAD_KEY_MAP_SIZE; i++)
    {
        int button_index = g_GamepadKeyMap[i].bluepad32_button;
        PALKEY pal_key = g_GamepadKeyMap[i].pal_key;

        // Get current button state
        unsigned char current_state = Bluepad32_GetButtonState(button_index) ? 1 : 0;
        unsigned char last_state = g_last_button_states[button_index];

        // Detect button press (edge detection)
        if (current_state && !last_state)
        {
            // Debug output for button press
            const char* buttonNames[] = {
                "D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right",
                "A Button", "B Button", "X Button", "Y Button",
                "L1 Button", "R1 Button", "L2 Button", "R2 Button",
                "Back Button", "Home Button", "System Button"
            };
            
            if (button_index >= 0 && button_index < BLUEPAD32_BUTTON_COUNT) {
                printf("🎮 Button Pressed: %s\n", buttonNames[button_index]);
            }
            
            PAL_KeyDown(pal_key);
        }
        // Detect button release
        else if (!current_state && last_state)
        {
            PAL_KeyUp(pal_key);
        }

        // Update last state
        g_last_button_states[button_index] = current_state;
    }

    // Process analog stick as direction input
    int analog_x, analog_y;
    Bluepad32_GetAnalogStick(&analog_x, &analog_y);

    // Only update if analog state changed
    if (analog_x != g_last_analog_x || analog_y != g_last_analog_y)
    {
        PALDIRECTION analog_dir = kDirUnknown;

        // Convert analog to direction
        if (analog_x == 1 && analog_y >= 0)
        {
            analog_dir = kDirEast; // Right
        }
        else if (analog_x == -1 && analog_y <= 0)
        {
            analog_dir = kDirWest; // Left
        }
        else if (analog_y == 1 && analog_x <= 0)
        {
            analog_dir = kDirSouth; // Down
        }
        else if (analog_y == -1 && analog_x >= 0)
        {
            analog_dir = kDirNorth; // Up
        }
        else
        {
            analog_dir = kDirUnknown;
        }

        // Set direction input
        PAL_SetDirInput(analog_dir);

        // Update last analog state
        g_last_analog_x = analog_x;
        g_last_analog_y = analog_y;
    }
    return 0;
}
#endif /* ARDUINO_ARCH_ESP32 && PAL_ESP32_BACKEND_HW */
