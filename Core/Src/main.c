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
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void system_init();
void DisplayTime();
void UpdateTime();
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
	lcd_show_picture(0, 0, 240, 180, gImage_a);
	lcd_show_picture(100, 220, 140, 80, gImage_b);

	while (1) {
		ds3231_read_time();
		DisplayTime();
		
		// Add a delay to avoid overwhelming the CPU and flickering the LCD.
		// Updating the display every 500ms is sufficient.
		HAL_Delay(500);
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

void DisplayTime() {
    // Array for abbreviated day names. Note: DS3231 day is 1-7 (Sun-Sat).
    const char* day_names[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
	const uint8_t TIME_FONT_SIZE = 24;
	const uint8_t INFO_FONT_SIZE = 16;
	const int y_offset = 20;   // Khoảng cách giữa các dòng
	int y = 180;               // y bắt đầu cho giờ

    // Hiển thị Giờ : Phút : Giây
    // Căn giữa dòng thời gian
    int time_str_width = 2 * (TIME_FONT_SIZE/2) + 1 * (TIME_FONT_SIZE/2) + 2 * (TIME_FONT_SIZE/2) + 1 * (TIME_FONT_SIZE/2) + 2 * (TIME_FONT_SIZE/2);
    int x_time_start = (lcddev.width - time_str_width) / 2;
    lcd_show_int_num(x_time_start, y, ds3231_hours, 2, GREEN, BLACK, TIME_FONT_SIZE);
    lcd_show_string(x_time_start + 2*(TIME_FONT_SIZE/2), y, ":", GREEN, BLACK, TIME_FONT_SIZE, 0);
    lcd_show_int_num(x_time_start + 3*(TIME_FONT_SIZE/2), y, ds3231_min, 2, GREEN, BLACK, TIME_FONT_SIZE);
    lcd_show_string(x_time_start + 5*(TIME_FONT_SIZE/2), y, ":", GREEN, BLACK, TIME_FONT_SIZE, 0);
    lcd_show_int_num(x_time_start + 6*(TIME_FONT_SIZE/2), y, ds3231_sec, 2, GREEN, BLACK, TIME_FONT_SIZE);

    // Clear the date area before drawing
    lcd_fill(0, y + 30, 80, 140, BLACK); // Xóa vùng dưới dòng thời gian

    // Display Day of Week
    y += 30;  // tăng y
    lcd_show_string(10, y, "Day:", YELLOW, BLACK, INFO_FONT_SIZE, 0);
    if(ds3231_day >= 1 && ds3231_day <= 7) {
        lcd_show_string(70, y, (char*)day_names[ds3231_day-1], YELLOW, BLACK, INFO_FONT_SIZE, 0);
    }

    // Display Date
    y += y_offset;
    lcd_show_string(10, y, "Date:", YELLOW, BLACK, INFO_FONT_SIZE, 0);
    lcd_show_int_num(70, y, ds3231_date, 2, YELLOW, BLACK, INFO_FONT_SIZE);
    y += y_offset;
    lcd_show_string(10, y, "Month:", YELLOW, BLACK, INFO_FONT_SIZE, 0);
    lcd_show_int_num(70, y, ds3231_month, 2, YELLOW, BLACK, INFO_FONT_SIZE);

    // Display Year
    y += y_offset;
    lcd_show_string(10, y, "Year:", YELLOW, BLACK, INFO_FONT_SIZE, 0);
    lcd_show_int_num(70, y, ds3231_year + 2000, 4, YELLOW, BLACK, INFO_FONT_SIZE);
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
