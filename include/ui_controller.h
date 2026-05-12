#ifndef UI_CONTROLLER_H
#define UI_CONTROLLER_H

/* --- Function Prototypes --- */
/**
 * @brief Processes joystick input and updates global display states.
 * Call this periodically (e.g., every 50ms-100ms) from the main loop.
 */
void ui_controller_update(void);

#endif // UI_CONTROLLER_H