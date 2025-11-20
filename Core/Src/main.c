/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"
#include "fsmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "software_timer.h"
#include "lcd.h"
#include "ds3231.h"
#include "picture.h" // Tạm thời không sử dụng chức năng hiển thị ảnh
#include "button.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// State machine for clock modes
typedef enum {
    NORMAL,
    ADJUST_INIT,
    ADJUST_HOUR,
    ADJUST_DAY,
    ADJUST_MIN,
    ADJUST_DATE,
    ADJUST_MONTH,
    ADJUST_YEAR
} ClockState;

ClockState clockState = NORMAL;

// Temporary variables for time adjustment
int8_t temp_hours, temp_min, temp_day, temp_date, temp_month, temp_year;

// Static variables to store previous values to update only changed parts
static int8_t prev_sec = -1, prev_min = -1, prev_hours = -1; // For NORMAL mode
static int8_t prev_day = -1, prev_date = -1, prev_month = -1, prev_year = -1;
static int8_t prev_temp_hours = -1, prev_temp_min = -1, prev_temp_date = -1, prev_temp_month = -1, prev_temp_year = -1; // For ADJUST mode


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void system_init();
void DisplayTime();
void UpdateTime();
void handle_adjust_mode();

void RestoreBackground(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// This function restores a rectangular part of the background image from the gImage_a buffer.
void RestoreBackground(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    const int screen_width = 240;
    uint32_t source_offset;
    uint16_t i, j;

    // lcd_draw_point sets the address for each pixel, so we iterate and call it.
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            source_offset = ((y + j) * screen_width + (x + i)) * 2;
            uint16_t color = (gImage_a[source_offset] << 8) | gImage_a[source_offset + 1];
            lcd_draw_point(x + i, y + j, color);
        }
    }
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_TIM2_Init();
	MX_SPI1_Init();
	MX_FSMC_Init();
	MX_I2C1_Init();
	MX_TIM4_Init();
	/* USER CODE BEGIN 2 */
	system_init();
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	// The UpdateTime() function should only be called once to set the initial time for the DS3231 module.
	// After the first run, you should comment it out to allow the RTC to keep its own time.
	// UpdateTime();

	// Xóa màn hình về màu đen khi khởi động
	lcd_clear(BLACK);
	
	// Ví dụ hiển thị ảnh với kích thước mới (200x45)
	// Bạn cần đảm bảo mảng gImage_a đã được tạo lại với kích thước tương ứng (18000 bytes)
	// lcd_show_picture(x, y, width, lenth, image_array);
	lcd_show_picture(0, 0, 240, 320, gImage_a);

	timer2_set(50); // Set timer to 50ms to match button scan frequency

	while (1) {
		if(timer2_flag){
			timer2_flag = 0;
			button_scan(); // Assuming you have this function in button.c

			if (clockState == NORMAL) {
				if (is_button_pressed(MODE_BUTTON)) { // Assuming MODE_BUTTON is defined (e.g., index 0)
					clockState = ADJUST_INIT;
					// Force a full redraw on next DisplayTime call by invalidating a prev value
					// This will trigger the redraw logic in DisplayTime
					prev_sec = -1;
					ds3231_read_time(); // Read current time before adjusting
				}
				ds3231_read_time(); // Read time only in normal mode
			} else {
				handle_adjust_mode();
			}
			DisplayTime(); // Always display, DisplayTime will handle different states
		}
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 168;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */
void system_init() {
	HAL_GPIO_WritePin(OUTPUT_Y0_GPIO_Port, OUTPUT_Y0_Pin, 0);
	HAL_GPIO_WritePin(OUTPUT_Y1_GPIO_Port, OUTPUT_Y1_Pin, 0);
	HAL_GPIO_WritePin(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin, 0);

	lcd_init();
	ds3231_init();
	timer2_init();
	// timer2_set(500); // Moved to main
}

void UpdateTime() {
	ds3231_write(ADDRESS_YEAR, 23);
	ds3231_write(ADDRESS_MONTH, 10);
	ds3231_write(ADDRESS_DATE, 20);
	ds3231_write(ADDRESS_DAY, 6);
	ds3231_write(ADDRESS_HOUR, 20);
	ds3231_write(ADDRESS_MIN, 11);
	ds3231_write(ADDRESS_SEC, 23);
}

void handle_adjust_mode() {

    switch (clockState) {
        case ADJUST_INIT:
            // Load current time into temporary variables
            temp_hours = ds3231_hours;
            temp_min = ds3231_min;
            temp_day = ds3231_day;
            temp_date = ds3231_date;
            temp_month = ds3231_month;
            temp_year = ds3231_year;
            clockState = ADJUST_HOUR;
            break;

        case ADJUST_HOUR:
            if (is_button_pressed(UP_BUTTON) || is_button_long_pressed(UP_BUTTON)) temp_hours = (temp_hours + 1) % 24;
            if (is_button_pressed(DOWN_BUTTON) || is_button_long_pressed(DOWN_BUTTON)) { temp_hours--; if(temp_hours < 0) temp_hours = 23; }
            if (is_button_pressed(SET_BUTTON)) clockState = ADJUST_MIN;
            break;

        case ADJUST_MIN:
            if (is_button_pressed(UP_BUTTON) || is_button_long_pressed(UP_BUTTON)) temp_min = (temp_min + 1) % 60;
            if (is_button_pressed(DOWN_BUTTON) || is_button_long_pressed(DOWN_BUTTON)) { temp_min--; if(temp_min < 0) temp_min = 59; }
            if (is_button_pressed(SET_BUTTON)) clockState = ADJUST_DAY;
            break;

        case ADJUST_DAY:
            if (is_button_pressed(UP_BUTTON) || is_button_long_pressed(UP_BUTTON)) { temp_day++; if(temp_day > 7) temp_day = 1; }
            if (is_button_pressed(DOWN_BUTTON) || is_button_long_pressed(DOWN_BUTTON)) { temp_day--; if(temp_day < 1) temp_day = 7; }
            if (is_button_pressed(SET_BUTTON)) clockState = ADJUST_DATE;
            break;

        case ADJUST_DATE:
            if (is_button_pressed(UP_BUTTON) || is_button_long_pressed(UP_BUTTON)) { temp_date++; if(temp_date > 31) temp_date = 1; }
            if (is_button_pressed(DOWN_BUTTON) || is_button_long_pressed(DOWN_BUTTON)) { temp_date--; if(temp_date < 1) temp_date = 31; }
            if (is_button_pressed(SET_BUTTON)) clockState = ADJUST_MONTH;
            break;

        case ADJUST_MONTH:
            if (is_button_pressed(UP_BUTTON) || is_button_long_pressed(UP_BUTTON)) { temp_month++; if(temp_month > 12) temp_month = 1; }
            if (is_button_pressed(DOWN_BUTTON) || is_button_long_pressed(DOWN_BUTTON)) { temp_month--; if(temp_month < 1) temp_month = 12; }
            if (is_button_pressed(SET_BUTTON)) clockState = ADJUST_YEAR;
            break;

        case ADJUST_YEAR:
            if (is_button_pressed(UP_BUTTON) || is_button_long_pressed(UP_BUTTON)) temp_year = (temp_year + 1) % 100;
            if (is_button_pressed(DOWN_BUTTON) || is_button_long_pressed(DOWN_BUTTON)) { temp_year--; if(temp_year < 0) temp_year = 99; }
            if (is_button_pressed(SET_BUTTON)) {
                // Save all temp values to RTC
                ds3231_write(ADDRESS_HOUR, temp_hours);
                ds3231_write(ADDRESS_MIN, temp_min);
                ds3231_write(ADDRESS_DATE, temp_date);
                ds3231_write(ADDRESS_MONTH, temp_month);
                ds3231_write(ADDRESS_YEAR, temp_year);
                // Exit adjust mode
                clockState = NORMAL;
                // Force a full redraw on next DisplayTime call
				ds3231_sec = -1;
            }
            break;

        default:
            clockState = NORMAL;
            break;
    }
}

void DisplayTime() {
	static uint8_t blink_counter = 0;
	blink_counter = (blink_counter + 1) % 10; // 50ms * 10 = 500ms cycle for 2Hz blink

    const char* day_names[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    const uint8_t FONT_SIZE = 16;
    char buf[25];

    // Define coordinates and dimensions for each field
    const int x_time = 40;
    const int y_time = 20;
    const int line_height = FONT_SIZE + 8;
	const int text_width = 150;

	// Use different variables depending on the mode
	int8_t current_hours = (clockState == NORMAL) ? ds3231_hours : temp_hours;
	int8_t current_min = (clockState == NORMAL) ? ds3231_min : temp_min;
	int8_t current_sec = (clockState == NORMAL) ? ds3231_sec : 0;
	int8_t current_day = (clockState == NORMAL) ? ds3231_day : temp_day;
	int8_t current_date = (clockState == NORMAL) ? ds3231_date : temp_date;
	int8_t current_month = (clockState == NORMAL) ? ds3231_month : temp_month;
	int8_t current_year = (clockState == NORMAL) ? ds3231_year : temp_year;

	int8_t* p_prev_hours = (clockState == NORMAL) ? &prev_hours : &prev_temp_hours;
	int8_t* p_prev_min = (clockState == NORMAL) ? &prev_min : &prev_temp_min;
	int8_t* p_prev_sec = (clockState == NORMAL) ? &prev_sec : &prev_sec; // Use normal prev_sec
	int8_t* p_prev_day = (clockState == NORMAL) ? &prev_day : &prev_day; // Day is also adjusted now
	int8_t* p_prev_date = (clockState == NORMAL) ? &prev_date : &prev_temp_date;
	int8_t* p_prev_month = (clockState == NORMAL) ? &prev_month : &prev_temp_month;
	int8_t* p_prev_year = (clockState == NORMAL) ? &prev_year : &prev_temp_year;

	// On mode change, invalidate all previous values to force redraw
	static ClockState prevState = NORMAL;
	if (clockState != prevState) {
		// When switching modes, invalidate all previous values to force a full redraw
		// and clear the entire display area once.
		prev_sec = -1; prev_min = -1; prev_hours = -1;
		prev_day = -1; prev_date = -1; prev_month = -1; prev_year = -1;
		prev_temp_hours = -1; prev_temp_min = -1; prev_temp_date = -1; prev_temp_month = -1; prev_temp_year = -1;

		// Clear the entire display area when switching modes
		RestoreBackground(x_time, y_time, text_width, (FONT_SIZE + line_height) * 5);

		prevState = clockState;
	}

	// --- Time Display ---
	// Redraw the entire time line if any part of it changes or is being adjusted
	if (current_hours != *p_prev_hours || current_min != *p_prev_min || (clockState == NORMAL && current_sec != *p_prev_sec) || clockState == ADJUST_HOUR || clockState == ADJUST_MIN) {
		RestoreBackground(x_time, y_time, text_width, FONT_SIZE);

		if (clockState != ADJUST_HOUR || blink_counter < 5) {
			sprintf(buf, "%02d", current_hours);
			lcd_show_string(x_time, y_time, buf, (clockState == ADJUST_HOUR) ? RED : GREEN, 0, FONT_SIZE, 1);
		} else {
			RestoreBackground(x_time, y_time, 2 * 8, FONT_SIZE); // 2 chars * 8 pixels/char
		}
		*p_prev_hours = current_hours;

		if (clockState != ADJUST_MIN || blink_counter < 5) {
			sprintf(buf, ":%02d", current_min);
			lcd_show_string(x_time + 2*8, y_time, buf, (clockState == ADJUST_MIN) ? RED : GREEN, 0, FONT_SIZE, 1);
		} else {
			RestoreBackground(x_time + 3*8, y_time, 2 * 8, FONT_SIZE);
		}
		*p_prev_min = current_min;

		if (clockState == NORMAL) { // Only show seconds in normal mode
			sprintf(buf, ":%02d", current_sec);
			lcd_show_string(x_time + 5*8, y_time, buf, GREEN, 0, FONT_SIZE, 1);
		}
		*p_prev_sec = current_sec;
	}


    // Day
    if (current_day != *p_prev_day || clockState == ADJUST_DAY) {
		if (clockState != ADJUST_DAY || blink_counter < 5) {
        sprintf(buf, "Day: %s", (current_day >= 1 && current_day <= 7) ? day_names[current_day - 1] : "N/A");
			lcd_show_string(x_time, y_time + line_height, buf, (clockState == ADJUST_DAY) ? RED : YELLOW, 0, FONT_SIZE, 1);
		} else {
			RestoreBackground(x_time, y_time + line_height, text_width, FONT_SIZE + 2);
		}
        *p_prev_day = current_day;
    }

    // Date
    if (current_date != *p_prev_date || clockState == ADJUST_DATE) {
		// Clear the area first to prevent artifacts
		RestoreBackground(x_time, y_time + 2 * line_height, text_width, FONT_SIZE + 2);
		if (clockState != ADJUST_DATE || blink_counter < 5) {
			sprintf(buf, "Date: %02d", current_date);
			lcd_show_string(x_time, y_time + 2 * line_height, buf, (clockState == ADJUST_DATE) ? RED : YELLOW, 0, FONT_SIZE, 1);
		} else {
			RestoreBackground(x_time, y_time + 2 * line_height, text_width, FONT_SIZE + 2);
		}
        *p_prev_date = current_date;
    }

    // Month
    if (current_month != *p_prev_month || clockState == ADJUST_MONTH) {
		RestoreBackground(x_time, y_time + 3 * line_height, text_width, FONT_SIZE + 2);
		if (clockState != ADJUST_MONTH || blink_counter < 5) {
			sprintf(buf, "Month: %02d", current_month);
			lcd_show_string(x_time, y_time + 3 * line_height, buf, (clockState == ADJUST_MONTH) ? RED : YELLOW, 0, FONT_SIZE, 1);
		} else {
			RestoreBackground(x_time, y_time + 3 * line_height, text_width, FONT_SIZE + 2);
		}
        *p_prev_month = current_month;
    }

    // Year
    if (current_year != *p_prev_year || clockState == ADJUST_YEAR) {
		RestoreBackground(x_time, y_time + 4 * line_height, text_width, FONT_SIZE + 2);
		if (clockState != ADJUST_YEAR || blink_counter < 5) {
			sprintf(buf, "Year: %04d", current_year + 2000);
			lcd_show_string(x_time, y_time + 4 * line_height, buf, (clockState == ADJUST_YEAR) ? RED : YELLOW, 0, FONT_SIZE, 1);
		} else {
			RestoreBackground(x_time, y_time + 4 * line_height, text_width, FONT_SIZE);
		}
        *p_prev_year = current_year;
    }
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
