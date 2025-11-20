/*
 * button.h
 */

#ifndef INC_BUTTON_H_
#define INC_BUTTON_H_

#include "main.h"

// Define button indices
#define DOWN_BUTTON     7  // Button for decreasing value
#define UP_BUTTON       3  // Button for increasing value
#define MODE_BUTTON     0  // Button for changing mode
#define SET_BUTTON      12  // Button for setting/confirming value

// Define button press durations (in terms of scan cycles, e.g., 50ms per cycle)
#define DURATION_FOR_AUTO_INCREASING  20 // 20 * 50ms = 1s
#define BUTTON_IS_PRESSED_S           (button_count[i] > DURATION_FOR_AUTO_INCREASING)

/* Functions */
void button_init(void);
void button_scan(void);

// Returns 1 if the button is pressed (single press)
int is_button_pressed(int i);

// Returns 1 if the button is held for a short duration (e.g., > 1s)
int is_button_pressed_s(int i);

// Returns 1 if the button is held long enough for auto-repeat
int is_button_long_pressed(int i);

#endif /* INC_BUTTON_H_ */