#ifndef _BLUEPAD32_API_H_
#define _BLUEPAD32_API_H_

#ifdef __cplusplus
extern "C"
{
#endif

    // Bluepad32 C API wrapper functions
    // This header provides a C interface to the C++ Bluepad32 library

    /**
     * Button mapping indices for Bluepad32_GetButtonState()
     */
    #define BLUEPAD32_BUTTON_DPAD_UP    0
    #define BLUEPAD32_BUTTON_DPAD_DOWN  1
    #define BLUEPAD32_BUTTON_DPAD_LEFT  2
    #define BLUEPAD32_BUTTON_DPAD_RIGHT 3
    #define BLUEPAD32_BUTTON_A          4
    #define BLUEPAD32_BUTTON_B          5
    #define BLUEPAD32_BUTTON_X          6
    #define BLUEPAD32_BUTTON_Y          7
    #define BLUEPAD32_BUTTON_L1         8
    #define BLUEPAD32_BUTTON_R1         9
    #define BLUEPAD32_BUTTON_L2         10
    #define BLUEPAD32_BUTTON_R2         11
    #define BLUEPAD32_BUTTON_BACK       12
    #define BLUEPAD32_BUTTON_HOME       13
    #define BLUEPAD32_BUTTON_SYSTEM     14
    #define BLUEPAD32_BUTTON_COUNT      15

    /**
     * Initialize Bluepad32 system
     * @return 0 on success, non-zero on error
     */
    int Bluepad32_Init(void);

    /**
     * Shutdown Bluepad32 system
     */
    void Bluepad32_Shutdown(void);

    /**
     * Update Bluepad32 controller states
     * Call this function regularly in your main loop
     */
    void Bluepad32_Update(void);

    /**
     * Check if any controller is connected
     * @return 1 if connected, 0 if not
     */
    int Bluepad32_IsControllerConnected(void);

    /**
     * Get button state for specific button
     * @param button_index Button index (0-14, see mapping table)
     * @return 1 if pressed, 0 if not pressed
     */
    int Bluepad32_GetButtonState(int button_index);

    /**
     * Get analog stick direction
     * @param stick_x Pointer to receive X axis value (-1, 0, 1)
     * @param stick_y Pointer to receive Y axis value (-1, 0, 1)
     */
    void Bluepad32_GetAnalogStick(int *stick_x, int *stick_y);

    /**
     * Wait for gamepad connection and button confirmation
     * This function blocks until a gamepad is connected and all required buttons are pressed
     * @return 1 on success, 0 on failure
     */
    int Bluepad32_WaitForGamepadConnection(void);

    /**
     * Try to connect gamepad in non-blocking mode
     * Call this regularly to attempt connection without blocking the main thread
     * @return 1 if gamepad newly connected, -1 if disconnected, 0 if no state change
     */
    int Bluepad32_TryConnect(void);

    /**
     * Check if gamepad is ready (connected and confirmed)
     * @return 1 if ready, 0 if not ready
     */
    int Bluepad32_IsGamepadReady(void);

    /**
     * Handle gamepad disconnection and reconnection
     * Call this when you detect a disconnection during gameplay
     */
    void Bluepad32_HandleReconnection(void);

#ifdef __cplusplus
}
#endif

#endif // _BLUEPAD32_API_H_