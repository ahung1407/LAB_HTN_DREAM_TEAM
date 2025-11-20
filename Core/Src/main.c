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
    ADJUST_MIN,
    ADJUST_DATE,
    ADJUST_MONTH,
    ADJUST_YEAR
} ClockState;

ClockState clockState = NORMAL;

// Temporary variables for time adjustment
int8_t temp_hours, temp_min, temp_date, temp_month, temp_year;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void system_init();
void DisplayTime();
void UpdateTime();
void handle_adjust_mode();
void display_blinking_value(int value, int x, int y, int width, int height, int is_year);

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
				}
				ds3231_read_time(); // Read time only in normal mode
				DisplayTime();
			} else {
				handle_adjust_mode();
			}
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
    static uint8_t blink_counter = 0; // 50ms tick
    blink_counter = (blink_counter + 1) % 10; // Blink at 2Hz (50ms * 10 = 500ms cycle)

    switch (clockState) {
        case ADJUST_INIT:
            // Load current time into temporary variables
            temp_hours = ds3231_hours;
            temp_min = ds3231_min;
            temp_date = ds3231_date;
            temp_month = ds3231_month;
            temp_year = ds3231_year;
            clockState = ADJUST_HOUR;
            break;

        case ADJUST_HOUR:
            if (is_button_pressed(UP_BUTTON) || is_button_long_pressed(UP_BUTTON)) temp_hours = (temp_hours + 1) % 24;
            if (is_button_pressed(SET_BUTTON)) clockState = ADJUST_MIN;
            if (blink_counter < 5) display_blinking_value(temp_hours, 40, 20, 32, 16, 0); else RestoreBackground(40, 20, 32, 16);
            break;

        case ADJUST_MIN:
            if (is_button_pressed(UP_BUTTON) || is_button_long_pressed(UP_BUTTON)) temp_min = (temp_min + 1) % 60;
            if (is_button_pressed(SET_BUTTON)) clockState = ADJUST_DATE;
            if (blink_counter < 5) display_blinking_value(temp_min, 80, 20, 32, 16, 0); else RestoreBackground(80, 20, 32, 16);
            break;

        case ADJUST_DATE:
            if (is_button_pressed(UP_BUTTON) || is_button_long_pressed(UP_BUTTON)) { if(++temp_date > 31) temp_date = 1; }
            if (is_button_pressed(SET_BUTTON)) clockState = ADJUST_MONTH;
            if (blink_counter < 5) display_blinking_value(temp_date, 110, 20 + 2 * (16 + 8), 32, 16, 0); else RestoreBackground(110, 20 + 2 * (16 + 8), 32, 16);
            break;

        case ADJUST_MONTH:
            if (is_button_pressed(UP_BUTTON) || is_button_long_pressed(UP_BUTTON)) { if(++temp_month > 12) temp_month = 1; }
            if (is_button_pressed(SET_BUTTON)) clockState = ADJUST_YEAR;
            if (blink_counter < 5) display_blinking_value(temp_month, 120, 20 + 3 * (16 + 8), 32, 16, 0); else RestoreBackground(120, 20 + 3 * (16 + 8), 32, 16);
            break;

        case ADJUST_YEAR:
            if (is_button_pressed(UP_BUTTON) || is_button_long_pressed(UP_BUTTON)) temp_year = (temp_year + 1) % 100;
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
            if (blink_counter < 5) display_blinking_value(temp_year, 110, 20 + 4 * (16 + 8), 64, 16, 1); else RestoreBackground(110, 20 + 4 * (16 + 8), 64, 16);
            break;

        default:
            clockState = NORMAL;
            break;
    }

    // If in any adjust sub-state, display the temporary (non-blinking) values
    if (clockState > ADJUST_INIT) {
        char buf[25];
        const uint8_t FONT_SIZE = 16;
        const int x_start = 40;
        const int y_start = 20;
        const int line_height = FONT_SIZE + 8;

        // Display non-blinking parts
        if (clockState != ADJUST_HOUR) {
            sprintf(buf, "%02d", temp_hours);
            lcd_show_string(x_start, y_start, buf, GREEN, 0, FONT_SIZE, 1);
        }
        if (clockState != ADJUST_MIN) {
            sprintf(buf, ":%02d", temp_min);
            lcd_show_string(x_start + 32, y_start, buf, GREEN, 0, FONT_SIZE, 1);
        }
        if (clockState != ADJUST_DATE) {
            sprintf(buf, "Date: %02d", temp_date);
            lcd_show_string(x_start, y_start + 2 * line_height, buf, YELLOW, 0, FONT_SIZE, 1);
        }
        if (clockState != ADJUST_MONTH) {
            sprintf(buf, "Month: %02d", temp_month);
            lcd_show_string(x_start, y_start + 3 * line_height, buf, YELLOW, 0, FONT_SIZE, 1);
        }
        if (clockState != ADJUST_YEAR) {
            sprintf(buf, "Year: %04d", temp_year + 2000);
            lcd_show_string(x_start, y_start + 4 * line_height, buf, YELLOW, 0, FONT_SIZE, 1);
        }
    }
}

void display_blinking_value(int value, int x, int y, int width, int height, int is_year) {
    char buf[10];
    const uint8_t FONT_SIZE = 16;
    RestoreBackground(x, y, width, height);
    if (is_year) {
        sprintf(buf, "Year: %04d", value + 2000);
    } else {
        sprintf(buf, "%02d", value);
    }
    lcd_show_string(x, y, buf, RED, 0, FONT_SIZE, 1); // Use RED to indicate editing
}

void DisplayTime() {
    // Static variables to store previous values to update only changed parts
    static int8_t prev_sec = -1, prev_min = -1, prev_hours = -1;
    static int8_t prev_day = -1, prev_date = -1, prev_month = -1, prev_year = -1;

    const char* day_names[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    const uint8_t FONT_SIZE = 16;
    char buf[25];

    // Define coordinates and dimensions for each field
    const int x_start = 40;
    const int y_start = 20;
    const int line_height = FONT_SIZE + 8;
	const int text_width = 150;

    // Time: 00:00:00
    if (ds3231_hours != prev_hours || ds3231_min != prev_min || ds3231_sec != prev_sec) {
        if(prev_sec != -1) RestoreBackground(x_start, y_start, text_width, FONT_SIZE); // Avoid clearing on first run
        sprintf(buf, "%02d:%02d:%02d", ds3231_hours, ds3231_min, ds3231_sec);
        lcd_show_string(x_start, y_start, buf, GREEN, 0, FONT_SIZE, 1);
        prev_hours = ds3231_hours;
        prev_min = ds3231_min;
        prev_sec = ds3231_sec;
    }

    // Day
    if (ds3231_day != prev_day) {
    	if(prev_day != -1) RestoreBackground(x_start, y_start + line_height, text_width, FONT_SIZE);
        sprintf(buf, "Day: %s", (ds3231_day >= 1 && ds3231_day <= 7) ? day_names[ds3231_day - 1] : "N/A");
        lcd_show_string(x_start, y_start + line_height, buf, YELLOW, 0, FONT_SIZE, 1);
        prev_day = ds3231_day;
    }

    // Date
    if (ds3231_date != prev_date || prev_sec == -1) { // Also draw on first run
    	if(prev_date != -1) RestoreBackground(x_start, y_start + 2 * line_height, text_width, FONT_SIZE);
        sprintf(buf, "Date: %02d", ds3231_date);
        lcd_show_string(x_start, y_start + 2 * line_height, buf, YELLOW, 0, FONT_SIZE, 1);
        prev_date = ds3231_date;
    }

    // Month
    if (ds3231_month != prev_month || prev_sec == -1) {
    	if(prev_month != -1) RestoreBackground(x_start, y_start + 3 * line_height, text_width, FONT_SIZE);
        sprintf(buf, "Month: %02d", ds3231_month);
        lcd_show_string(x_start, y_start + 3 * line_height, buf, YELLOW, 0, FONT_SIZE, 1);
        prev_month = ds3231_month;
    }

    // Year
    if (ds3231_year != prev_year || prev_sec == -1) {
    	if(prev_year != -1) RestoreBackground(x_start, y_start + 4 * line_height, text_width, FONT_SIZE);
        sprintf(buf, "Year: %04d", ds3231_year + 2000);
        lcd_show_string(x_start, y_start + 4 * line_height, buf, YELLOW, 0, FONT_SIZE, 1);
        prev_year = ds3231_year;
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
