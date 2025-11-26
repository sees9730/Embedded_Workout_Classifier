/* accelerometer.c
 * Simplified LSM303DLHC driver for STM32F411E-DISCO
 * Fixed config: 100Hz, ±8g, high-res, all axes enabled
 * Daphne Felt - ECEN 5613
 */

#include "accelerometer.h"
#include "stm32f4xx_hal.h"

// LSM303DLHC I2C address
#define LSM303_ADDR             (0x19 << 1)  // 0x32 after shift

// Register addresses
#define CTRL_REG1_A             0x20
#define CTRL_REG4_A             0x23
#define OUT_X_L_A               0x28

// Register values for fixed configuration
#define CTRL1_100HZ_ENABLED     0x57  // 100Hz, all axes enabled
#define CTRL4_8G_HIGHRES_BDU    0xA8  // ±8g, high-res, BDU enabled

// I2C timeout
#define I2C_TIMEOUT             100

// I2C handle
static I2C_HandleTypeDef hi2c1;

/* Initialize I2C1 peripheral */
static void init_i2c(void) {
    // Enable clocks
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    // Configure GPIO: PB6=SCL, PB9=SDA
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &gpio);
    
    // Configure I2C1: 400kHz
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    
    HAL_I2C_Init(&hi2c1);
}

/* Write single register */
static inline void write_reg(uint8_t reg, uint8_t val) {
    HAL_I2C_Mem_Write(&hi2c1, LSM303_ADDR, reg, 
                      I2C_MEMADD_SIZE_8BIT, &val, 1, I2C_TIMEOUT);
}

/* Read multiple registers with auto-increment */
static inline void read_regs(uint8_t reg, uint8_t *buf, uint8_t len) {
    reg |= 0x80;  // Set auto-increment bit
    HAL_I2C_Mem_Read(&hi2c1, LSM303_ADDR, reg, 
                     I2C_MEMADD_SIZE_8BIT, buf, len, I2C_TIMEOUT);
}

/* Initialize accelerometer: 100Hz, ±8g, high-res */
bool Accel_Init(void) {
    // Initialize I2C
    init_i2c();
    HAL_Delay(10);
    
    // Test connection
    if (HAL_I2C_IsDeviceReady(&hi2c1, LSM303_ADDR, 3, I2C_TIMEOUT) != HAL_OK) {
        return false;
    }
    
    // Configure: 100Hz, all axes enabled
    write_reg(CTRL_REG1_A, CTRL1_100HZ_ENABLED);
    
    // Configure: ±8g, high-res, BDU
    write_reg(CTRL_REG4_A, CTRL4_8G_HIGHRES_BDU);
    
    HAL_Delay(10);
    return true;
}

/* Read X, Y, Z values (burst read) */
void Accel_ReadRaw(AccelRawData *data) {
    uint8_t buf[6];
    
    // Burst read 6 bytes: X_L, X_H, Y_L, Y_H, Z_L, Z_H
    read_regs(OUT_X_L_A, buf, 6);
    
    // Combine bytes (little endian, 12-bit left-justified)
    data->x = (int16_t)((buf[1] << 8) | buf[0]) >> 4;
    data->y = (int16_t)((buf[3] << 8) | buf[2]) >> 4;
    data->z = (int16_t)((buf[5] << 8) | buf[4]) >> 4;
}
