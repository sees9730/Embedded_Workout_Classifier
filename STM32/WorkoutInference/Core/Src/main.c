/* main.c - Smartwatch with X-CUBE-AI Workout Detection
 * Daphne Felt - ECEN 5613
 */

#include <stdbool.h>
#include <stdio.h>
#include "stm32f411xe.h"
#include "main.h"
#include "uart.h"
#include "workout_inference.h"
#include "accelerometer.h"

void delay(volatile uint32_t t) {
    while(t--);
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 192;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 8;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3);
}

void Error_Handler(void) {
    __disable_irq();
    while(1) {}
}

int main(void) {
	HAL_Init();
    SystemClock_Config();

    UART_Init();
    Accel_Init();

    // set up AI workout detections
    sendString("Initializing\r\n");
    if (!Workout_Init()) {
        sendString("ERROR: initialization failed\r\n");
    }
    sendStringGreen("initialized successfully\r\n");

    uint32_t accel_timer = 0;
    uint32_t last_inference = 0;
    AccelRawData accelData;

    while(1) {
        uint32_t now = HAL_GetTick();

        // Sample accelerometer every 10ms (100hz, same speed we trained the model with)
        if (now - accel_timer >= 10) {
            accel_timer = now;
            Accel_ReadRaw(&accelData);
            Workout_AddSample(accelData.x, accelData.y, accelData.z); // add to buffer

            // inference, max 1s at a time
            if (Workout_ShouldInfer() && (now - last_inference > 1000)) {

                WorkoutResult result;
                if (Workout_RunInference(&result)) {

                    // print out all the results
                    char buf[120];
                    sprintf(buf, "\n>>>> WORKOUT DETECTED: %s\r\n", Workout_GetName(result.predicted_class));
                    sendStringGreen(buf);

                    sprintf(buf, "    Confidence: %.1f%% \r\n", result.confidence);
                    sendString(buf);

                    // all class scores
                    sendString("    All scores: ");
                    for (int i = 0; i < NUM_CLASSES; i++) {
                        sprintf(buf, "%s:%.0f%% ", Workout_GetName((WorkoutClass)i), result.class_scores[i]);
                        sendString(buf);
                    }
                    sendString("\r\n\n");

                    last_inference = now;

                } else {
                    // sendString("Inference failed :(\r\n");
                }
            }

        }
    }
}
