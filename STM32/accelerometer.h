/* accelerometer.h
 * LSM303DLHC 3-axis accelerometer driver for STM32F411E-DISCO
 * Connected via I2C (I2C1: PB6=SCL, PB9=SDA)
 * Daphne Felt - ECEN 5613
 */

#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include <stdint.h>
#include <stdbool.h>

// LSM303DLHC I2C addresses
#define LSM303DLHC_ACC_I2C_ADDR     (0x19 << 1)  // Accelerometer (0x32 after shift)
#define LSM303DLHC_MAG_I2C_ADDR     (0x1E << 1)  // Magnetometer (0x3C after shift)

// Accelerometer register addresses
#define LSM303DLHC_CTRL_REG1_A      0x20
#define LSM303DLHC_CTRL_REG2_A      0x21
#define LSM303DLHC_CTRL_REG3_A      0x22
#define LSM303DLHC_CTRL_REG4_A      0x23
#define LSM303DLHC_CTRL_REG5_A      0x24
#define LSM303DLHC_CTRL_REG6_A      0x25
#define LSM303DLHC_REFERENCE_A      0x26
#define LSM303DLHC_STATUS_REG_A     0x27
#define LSM303DLHC_OUT_X_L_A        0x28
#define LSM303DLHC_OUT_X_H_A        0x29
#define LSM303DLHC_OUT_Y_L_A        0x2A
#define LSM303DLHC_OUT_Y_H_A        0x2B
#define LSM303DLHC_OUT_Z_L_A        0x2C
#define LSM303DLHC_OUT_Z_H_A        0x2D

// CTRL_REG1_A bits
#define LSM303DLHC_ODR_1HZ          0x10
#define LSM303DLHC_ODR_10HZ         0x20
#define LSM303DLHC_ODR_25HZ         0x30
#define LSM303DLHC_ODR_50HZ         0x40
#define LSM303DLHC_ODR_100HZ        0x50  // Use for 100Hz sampling
#define LSM303DLHC_ODR_200HZ        0x60
#define LSM303DLHC_ODR_400HZ        0x70
#define LSM303DLHC_ODR_1620HZ       0x80
#define LSM303DLHC_ODR_5376HZ       0x90

#define LSM303DLHC_LPEN             0x08  // Low power mode enable
#define LSM303DLHC_ZEN              0x04  // Z-axis enable
#define LSM303DLHC_YEN              0x02  // Y-axis enable
#define LSM303DLHC_XEN              0x01  // X-axis enable
#define LSM303DLHC_ALL_AXES         (LSM303DLHC_XEN | LSM303DLHC_YEN | LSM303DLHC_ZEN)

// CTRL_REG4_A bits - Full scale selection
#define LSM303DLHC_FULLSCALE_2G     0x00  // ±2g (default)
#define LSM303DLHC_FULLSCALE_4G     0x10  // ±4g
#define LSM303DLHC_FULLSCALE_8G     0x20  // ±8g
#define LSM303DLHC_FULLSCALE_16G    0x30  // ±16g

#define LSM303DLHC_BDU              0x80  // Block data update
#define LSM303DLHC_HR               0x08  // High resolution mode

// STATUS_REG_A bits
#define LSM303DLHC_ZYXDA            0x08  // X, Y, Z-axis new data available
#define LSM303DLHC_ZDA              0x04  // Z-axis new data available
#define LSM303DLHC_YDA              0x02  // Y-axis new data available
#define LSM303DLHC_XDA              0x01  // X-axis new data available

// I2C peripheral selection (typically I2C1 on STM32F411E-DISCO)
#define ACCEL_I2C                   I2C1
#define ACCEL_I2C_TIMEOUT           100   // ms

// Accelerometer data structures
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} AccelRawData;

typedef struct {
    float x;  // in g
    float y;  // in g
    float z;  // in g
} AccelData;

// Configuration structure
typedef struct {
    uint8_t odr;           // Output data rate
    uint8_t fullscale;     // Full scale range (±2g, ±4g, ±8g, ±16g)
    bool high_res;         // High resolution mode
    bool low_power;        // Low power mode
    bool bdu_enable;       // Block data update
} AccelConfig;

// Function prototypes

// Initialization
bool Accel_Init(void);
bool Accel_InitWithConfig(AccelConfig *config);
bool Accel_TestConnection(void);

// Raw data reading (16-bit integers)
void Accel_ReadRaw(AccelRawData *data);
void Accel_ReadRawX(int16_t *x);
void Accel_ReadRawY(int16_t *y);
void Accel_ReadRawZ(int16_t *z);

// Converted data reading (floats in g)
void Accel_Read(AccelData *data);
void Accel_ReadX(float *x);
void Accel_ReadY(float *y);
void Accel_ReadZ(float *z);

// Configuration
void Accel_SetODR(uint8_t odr);
void Accel_SetFullScale(uint8_t fullscale);
void Accel_EnableHighResolution(bool enable);
void Accel_EnableLowPower(bool enable);
void Accel_EnableAxes(uint8_t axes);

// Utility functions
bool Accel_DataReady(void);
void Accel_PowerDown(void);
void Accel_PowerUp(void);
float Accel_RawToG(int16_t raw_value);

// Low-level I2C functions (internal)
uint8_t Accel_ReadReg(uint8_t reg);
void Accel_WriteReg(uint8_t reg, uint8_t value);
void Accel_ReadMultipleRegs(uint8_t reg, uint8_t *buffer, uint8_t count);

#endif // ACCELEROMETER_H
