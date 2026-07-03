#include "../../backend_select.h"
#if defined(ARDUINO_ARCH_ESP32) && PAL_ESP32_BACKEND_HW
/*
 * Bluepad32 C++ API Implementation
 * 
 * This file provides a C interface wrapper around the C++ Bluepad32 library.
 * It handles all C++ interactions and exposes simple C functions.
 */

#include "bluepad32_api.h"
#include <Bluepad32.h>
#include <Arduino.h>

// LED control for status indication
#ifndef LED_BUILTIN
#define LED_BUILTIN 22
#endif

// Button verification feature control
// #define ENABLE_BUTTON_VERIFICATION

// Internal C++ implementation
namespace {
    // Controller storage
    ControllerPtr g_controllers[BP32_MAX_GAMEPADS];
    
    // Button state tracking
    bool g_button_states[BP32_MAX_GAMEPADS][BLUEPAD32_BUTTON_COUNT];
    bool g_last_button_states[BP32_MAX_GAMEPADS][BLUEPAD32_BUTTON_COUNT];
    
    // Analog stick state
    int g_analog_x[BP32_MAX_GAMEPADS];
    int g_analog_y[BP32_MAX_GAMEPADS];
    
    // Gamepad connection and confirmation state
    bool g_gamepad_connected = false;
    bool g_gamepad_confirmed = false;
    
    /**
     * Wait for all 8 required buttons to be pressed for gamepad confirmation
     */
    bool waitForButtonConfirmation() {
        // Track which buttons have been pressed
        bool buttonsPressed[8] = {false}; // [Up, Down, Left, Right, A, B, X, Y]
        int buttonIndices[8] = {
            BLUEPAD32_BUTTON_DPAD_UP, BLUEPAD32_BUTTON_DPAD_DOWN,
            BLUEPAD32_BUTTON_DPAD_LEFT, BLUEPAD32_BUTTON_DPAD_RIGHT,
            BLUEPAD32_BUTTON_A, BLUEPAD32_BUTTON_B,
            BLUEPAD32_BUTTON_X, BLUEPAD32_BUTTON_Y};

        // Show button test instructions
        Serial.println("=== Gamepad Button Test ===");
        Serial.println("Please press the following 8 buttons in order:");
        Serial.println("D-Pad: Up, Down, Left, Right");
        Serial.println("Action: A, B, X, Y");
        Serial.println("===========================");

        // Track button states and wait for stabilization
        bool lastButtonStates[8] = {false};
        int pressedCount = 0;

        unsigned long stabilizeStart = millis();
        while (millis() - stabilizeStart < 1000) { // Wait 1 second
            BP32.update();
            
            // Check if any controller is connected
            bool connected = false;
            for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
                if (g_controllers[i] != nullptr && g_controllers[i]->isConnected()) {
                    connected = true;
                    break;
                }
            }
            if (!connected) return false;
            
            delay(50);
        }

        while (pressedCount < 8) {
            BP32.update();

            // Check if any controller is connected
            bool connected = false;
            for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
                if (g_controllers[i] != nullptr && g_controllers[i]->isConnected()) {
                    connected = true;
                    break;
                }
            }
            if (!connected) return false;

            // Check each required button
            for (int i = 0; i < 8; i++) {
                bool currentState = false;
                // Get button state directly from first connected controller
                for (int ctrl = 0; ctrl < BP32_MAX_GAMEPADS; ctrl++) {
                    if (g_controllers[ctrl] != nullptr && g_controllers[ctrl]->isConnected()) {
                        ControllerPtr controller = g_controllers[ctrl];
                        
                        switch (buttonIndices[i]) {
                            case BLUEPAD32_BUTTON_DPAD_UP:    
                                currentState = ((controller->dpad() & 0x01) != 0) || (controller->axisY() < -300);
                                break;
                            case BLUEPAD32_BUTTON_DPAD_DOWN:  
                                currentState = ((controller->dpad() & 0x02) != 0) || (controller->axisY() > 300);
                                break;
                            case BLUEPAD32_BUTTON_DPAD_LEFT:  
                                currentState = ((controller->dpad() & 0x04) != 0) || (controller->axisX() < -300);
                                break;
                            case BLUEPAD32_BUTTON_DPAD_RIGHT: 
                                currentState = ((controller->dpad() & 0x08) != 0) || (controller->axisX() > 300);
                                break;
                            case BLUEPAD32_BUTTON_A:          
                                currentState = (controller->buttons() & 0x0001) != 0;
                                break;
                            case BLUEPAD32_BUTTON_B:          
                                currentState = (controller->buttons() & 0x0002) != 0;
                                break;
                            case BLUEPAD32_BUTTON_X:          
                                currentState = (controller->buttons() & 0x0004) != 0;
                                break;
                            case BLUEPAD32_BUTTON_Y:          
                                currentState = (controller->buttons() & 0x0008) != 0;
                                break;
                        }
                        break;
                    }
                }

                // Detect button press (transition from not pressed to pressed)
                if (currentState && !lastButtonStates[i] && !buttonsPressed[i]) {
                    buttonsPressed[i] = true;
                    pressedCount++;

                    // Show progress with button names
                    const char *buttonNames[8] = {"Up", "Down", "Left", "Right", "A", "B", "X", "Y"};
                    Serial.printf("✓ %s button confirmed (%d/8)\n", buttonNames[i], pressedCount);
                }

                lastButtonStates[i] = currentState;
            }

            // Blink LED during confirmation
            static unsigned long lastBlink = 0;
            static bool ledState = false;
            if (millis() - lastBlink > 250) {
                ledState = !ledState;
                digitalWrite(LED_BUILTIN, ledState);
                lastBlink = millis();
            }

            delay(50);
        }

        Serial.println("🎮 All button tests completed!");
        digitalWrite(LED_BUILTIN, HIGH);
        return true;
    }

    /**
     * Wait for Bluepad32 gamepad connection and button confirmation
     */
    bool waitForGamepadConnection() {
        Serial.println("Waiting for Bluetooth gamepad connection...");

        // Wait until a gamepad is connected
        bool connected = false;
        while (!connected) {
            BP32.update();

            // Check if any controller is connected
            for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
                if (g_controllers[i] != nullptr && g_controllers[i]->isConnected()) {
                    connected = true;
                    break;
                }
            }

            if (!connected) {
                // Blink built-in LED to indicate waiting
                static unsigned long lastBlink = 0;
                static bool ledState = false;

                if (millis() - lastBlink > 500) { // Blink every 500ms
                    ledState = !ledState;
                    digitalWrite(LED_BUILTIN, ledState);
                    lastBlink = millis();
                }

                delay(50); // Small delay to prevent tight loop
            }
        }

        Serial.println("✅ Gamepad connected!");

        // Wait a moment for controller to stabilize
        delay(500);

#if defined(ENABLE_BUTTON_VERIFICATION)
        // Wait for button confirmation
        while (!waitForButtonConfirmation()) {
            Serial.println("❌ Button test failed or gamepad disconnected");

            // Wait for reconnection
            connected = false;
            while (!connected) {
                BP32.update();
                
                for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
                    if (g_controllers[i] != nullptr && g_controllers[i]->isConnected()) {
                        connected = true;
                        break;
                    }
                }
                delay(100);
            }

            Serial.println("✅ Gamepad reconnected!");
            delay(500); // Stabilization delay
        }
#else
        Serial.println("Button verification disabled - gamepad ready to use!");
#endif

        g_gamepad_connected = true;
        g_gamepad_confirmed = true;
        return true;
    }
    
    // Callbacks for Bluepad32
    void onConnectedController(ControllerPtr ctl) {
        for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
            if (g_controllers[i] == nullptr) {
                g_controllers[i] = ctl;
                Serial.printf("Bluepad32: Controller connected to slot %d\n", i);
                break;
            }
        }
    }
    
    void onDisconnectedController(ControllerPtr ctl) {
        for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
            if (g_controllers[i] == ctl) {
                g_controllers[i] = nullptr;
                Serial.printf("Bluepad32: Controller disconnected from slot %d\n", i);
                break;
            }
        }
    }
    
    // Get button state from controller
    bool getControllerButtonState(ControllerPtr controller, int button_index) {
        if (!controller || !controller->isConnected()) {
            return false;
        }
        
        switch (button_index) {
            case BLUEPAD32_BUTTON_DPAD_UP:    
                return ((controller->dpad() & 0x01) != 0) || (controller->axisY() < -300);
            case BLUEPAD32_BUTTON_DPAD_DOWN:  
                return ((controller->dpad() & 0x02) != 0) || (controller->axisY() > 300);
            case BLUEPAD32_BUTTON_DPAD_LEFT:  
                return ((controller->dpad() & 0x04) != 0) || (controller->axisX() < -300);
            case BLUEPAD32_BUTTON_DPAD_RIGHT: 
                return ((controller->dpad() & 0x08) != 0) || (controller->axisX() > 300);
            case BLUEPAD32_BUTTON_A:          return (controller->buttons() & 0x0001) != 0;
            case BLUEPAD32_BUTTON_B:          return (controller->buttons() & 0x0002) != 0;
            case BLUEPAD32_BUTTON_X:          return (controller->buttons() & 0x0004) != 0;
            case BLUEPAD32_BUTTON_Y:          return (controller->buttons() & 0x0008) != 0;
            case BLUEPAD32_BUTTON_L1:         return (controller->buttons() & 0x0100) != 0;
            case BLUEPAD32_BUTTON_R1:         return (controller->buttons() & 0x0200) != 0;
            case BLUEPAD32_BUTTON_L2:         return (controller->buttons() & 0x0400) != 0;
            case BLUEPAD32_BUTTON_R2:         return (controller->buttons() & 0x0800) != 0;
            case BLUEPAD32_BUTTON_BACK:       return (controller->miscButtons() & 0x01) != 0;
            case BLUEPAD32_BUTTON_HOME:       return (controller->miscButtons() & 0x02) != 0;
            case BLUEPAD32_BUTTON_SYSTEM:     return (controller->miscButtons() & 0x04) != 0;
            default: return false;
        }
    }
    
    // Update button states for all controllers
    void updateButtonStates() {
        for (int ctrl = 0; ctrl < BP32_MAX_GAMEPADS; ctrl++) {
            if (g_controllers[ctrl] == nullptr) continue;
            
            // Copy current states to last states
            for (int btn = 0; btn < BLUEPAD32_BUTTON_COUNT; btn++) {
                g_last_button_states[ctrl][btn] = g_button_states[ctrl][btn];
                g_button_states[ctrl][btn] = getControllerButtonState(g_controllers[ctrl], btn);
            }
        }
    }
    
    // Update analog stick states
    void updateAnalogStates() {
        for (int ctrl = 0; ctrl < BP32_MAX_GAMEPADS; ctrl++) {
            if (g_controllers[ctrl] == nullptr) continue;
            
            ControllerPtr controller = g_controllers[ctrl];
            if (!controller->isConnected()) continue;
            
            // Get raw analog values
            int raw_x = controller->axisX();
            int raw_y = controller->axisY();
            
            // Convert to simplified direction (-1, 0, 1)
            const int threshold = 200;
            g_analog_x[ctrl] = (raw_x > threshold) ? 1 : ((raw_x < -threshold) ? -1 : 0);
            g_analog_y[ctrl] = (raw_y > threshold) ? 1 : ((raw_y < -threshold) ? -1 : 0);
        }
    }
}

// C API Implementation
extern "C" {

int Bluepad32_Init(void) {
    // Initialize LED pin for status indication
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW); // Start with LED off

    // Initialize controller arrays
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        g_controllers[i] = nullptr;
        g_analog_x[i] = 0;
        g_analog_y[i] = 0;
        
        for (int j = 0; j < BLUEPAD32_BUTTON_COUNT; j++) {
            g_button_states[i][j] = false;
            g_last_button_states[i][j] = false;
        }
    }
    
    // Initialize connection state
    g_gamepad_connected = false;
    g_gamepad_confirmed = false;
    
    // Setup Bluepad32
    BP32.setup(&onConnectedController, &onDisconnectedController);
    BP32.enableNewBluetoothConnections(true);
    
    Serial.println("Bluepad32: API initialized successfully");
    return 0;
}

void Bluepad32_Shutdown(void) {
    // Clear all controllers
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        g_controllers[i] = nullptr;
    }
    Serial.println("Bluepad32: API shutdown");
}

void Bluepad32_Update(void) {
    BP32.update();
    updateButtonStates();
    updateAnalogStates();
}

int Bluepad32_IsControllerConnected(void) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (g_controllers[i] != nullptr && 
            g_controllers[i]->isConnected()) {
            return 1;
        }
    }
    return 0;
}

int Bluepad32_GetButtonState(int button_index) {
    if (button_index < 0 || button_index >= BLUEPAD32_BUTTON_COUNT) {
        return 0;
    }
    
    // Check first connected controller
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (g_controllers[i] != nullptr && 
            g_controllers[i]->isConnected()) {
            return g_button_states[i][button_index] ? 1 : 0;
        }
    }
    return 0;
}

void Bluepad32_GetAnalogStick(int *stick_x, int *stick_y) {
    if (stick_x) *stick_x = 0;
    if (stick_y) *stick_y = 0;
    
    // Get from first connected controller
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (g_controllers[i] != nullptr && 
            g_controllers[i]->isConnected()) {
            if (stick_x) *stick_x = g_analog_x[i];
            if (stick_y) *stick_y = g_analog_y[i];
            return;
        }
    }
}

int Bluepad32_WaitForGamepadConnection(void) {
    return waitForGamepadConnection() ? 1 : 0;
}

int Bluepad32_TryConnect(void) {
    // Check if any controller is connected (non-blocking)
    bool connected = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (g_controllers[i] != nullptr && g_controllers[i]->isConnected()) {
            connected = true;
            break;
        }
    }
    
    if (connected && !g_gamepad_connected) {
        // New connection detected - this is a state change
        Serial.println("✅ Gamepad connected!");
        g_gamepad_connected = true;
        
        // Turn on LED to indicate successful connection
        digitalWrite(LED_BUILTIN, HIGH);
        
#if defined(ENABLE_BUTTON_VERIFICATION)
        // Skip button verification for now to keep it non-blocking
        // In a real implementation, you might want to do a quick verification
        g_gamepad_confirmed = true;
        Serial.println("Button verification skipped - gamepad ready to use!");
#else
        g_gamepad_confirmed = true;
        Serial.println("Button verification disabled - gamepad ready to use!");
#endif
        
        return 1; // Return 1 only when newly connected
    } else if (!connected && g_gamepad_connected) {
        // Disconnection detected - this is a state change
        Serial.println("⚠️  Gamepad disconnected!");
        g_gamepad_connected = false;
        g_gamepad_confirmed = false;
        return -1; // Return -1 for disconnection
    } else if (!connected) {
        // Still waiting for connection - blink LED
        static unsigned long lastBlink = 0;
        static bool ledState = false;
        
        if (millis() - lastBlink > 500) { // Blink every 500ms
            ledState = !ledState;
            digitalWrite(LED_BUILTIN, ledState);
            lastBlink = millis();
        }
        return 0; // Return 0 for no connection
    }
    
    // Already connected, no state change
    return 0;
}

int Bluepad32_IsGamepadReady(void) {
    if (!g_gamepad_connected || !g_gamepad_confirmed) {
        return 0;
    }
    
    // Check if any controller is still connected
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (g_controllers[i] != nullptr && g_controllers[i]->isConnected()) {
            return 1;
        }
    }
    return 0;
}

void Bluepad32_HandleReconnection(void) {
    bool connected = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (g_controllers[i] != nullptr && g_controllers[i]->isConnected()) {
            connected = true;
            break;
        }
    }
    
    if (!connected && g_gamepad_connected) {
        // Gamepad disconnected
        Serial.println("⚠️  Gamepad disconnected! Game continues without gamepad input.");
        g_gamepad_connected = false;
        g_gamepad_confirmed = false;

        // Blink LED to indicate waiting for reconnection (non-blocking)
        digitalWrite(LED_BUILTIN, LOW);
        
        // Don't block here - let the game continue running
        // The TryConnect function will handle reconnection in the background
    }
}

} // extern "C"
#endif /* ARDUINO_ARCH_ESP32 && PAL_ESP32_BACKEND_HW */
