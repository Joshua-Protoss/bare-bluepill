#ifndef INC_I2C_H
#define INC_I2C_H

#include "common.h"
#include "gpio.h"


// I2C Base Addresses
#define I2C1_BASE                           (PERIPHERAL_APB1_BASE + 0x5400U)       
#define I2C2_BASE                           (PERIPHERAL_APB1_BASE + 0x5800U)

typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t OAR1;
    volatile uint32_t OAR2;
    volatile uint32_t DR;
    volatile uint32_t SR1;
    volatile uint32_t SR2;
    volatile uint32_t CCR;
    volatile uint32_t TRISE;
} I2C_reg_t;

// I2C Instances
#define I2C1                                ((volatile I2C_reg_t *) I2C1_BASE)      // Default SCL: PB6, SDA: PB7
#define I2C2                                ((volatile I2C_reg_t *) I2C2_BASE)      // Default SCL: PB10, SDA: PB11

// CR1 bits
#define I2C_CR1_PE                          BIT(0)                                  // Peripheral enable
#define I2C_CR1_SWRST                       BIT(15)                                 // Software reset

typedef struct {
    uint8_t scl_pin;
    uint8_t sda_pin;
    GPIO_reg_t *port;
    uint32_t speed_hz;
} I2C_config_t;

// Function prototypes
void i2c_bitbang_init(const I2C_config_t *config);
bool i2c_write(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len);
bool i2c_read(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len);
bool i2c_probe(uint8_t addr);                                               // Scan for device

// ===== HARDWARE I2C FUNCTIONS =====
void i2c_hardware_init(void);
bool i2c_hardware_read(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len);

extern const I2C_config_t MAX30102_I2C_CFG;


#endif // INC_I2C_H