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
#define I2C_CR1_SMBUS                       BIT(1)                                  // SMBus mode
#define I2C_CR1_SMBTYPE                     BIT(3)                                  // SMBus type
#define I2C_CR1_ENARP                       BIT(4)                                  // ARP enable
#define I2C_CR1_ENPEC                       BIT(5)                                  // PEC enable
#define I2C_CR1_ENGC                        BIT(6)                                  // General call enable
#define I2C_CR1_NOSTRETCH                   BIT(7)                                  // Clock stretching disable (Slave mode)
#define I2C_CR1_START                       BIT(8)                                  // Start generation
#define I2C_CR1_STOP                        BIT(9)                                  // Stop generation
#define I2C_CR1_ACK                         BIT(10)                                 // Acknowledge enable
#define I2C_CR1_POS                         BIT(11)                                 // Acknowledge/PEC Position (for data reception)
#define I2C_CR1_PEC                         BIT(12)                                 // Packet error checking
#define I2C_CR1_ALERT                       BIT(13)                                 // SMBus alert
#define I2C_CR1_SWRST                       BIT(15)                                 // Software reset

// SR1 bits
#define I2C_SR1_SB                          BIT(0)                                  // Start bit (Master mode)
#define I2C_SR1_ADDR                        BIT(1)                                  // Address sent (master mode)/matched (slave mode)
#define I2C_SR1_BTF                         BIT(2)                                  // Byte transfer finished
#define I2C_SR1_ADD10                       BIT(3)                                  // 10-bit header sent (Master mode)
#define I2C_SR1_STOPF                       BIT(4)                                  // Stop detection (slave mode) 
#define I2C_SR1_RxNE                        BIT(6)                                  // Data register not empty (receivers)
#define I2C_SR1_TxE                         BIT(7)                                  // Data register empty (transmitters) 
#define I2C_SR1_BERR                        BIT(8)                                  // Bus error
#define I2C_SR1_ARLO                        BIT(9)                                  // Arbitration lost (master mode) 
#define I2C_SR1_AF                          BIT(10)                                 // Acknowledge failure
#define I2C_SR1_OVR                         BIT(11)                                 // Overrun/Underrun
#define I2C_SR1_PECERR                      BIT(12)                                 // PEC Error in reception
#define I2C_SR1_TIMEOUT                     BIT(14)                                 // Timeout or Tlow error
#define I2C_SR1_SMBALERT                    BIT(15)                                 // SMBus alert 

// SR2 bits
#define I2C_SR2_MSL                         BIT(0)                                  // Master/slave
#define I2C_SR2_BUSY                        BIT(1)                                  // Bus busy
#define I2C_SR1_TRA                         BIT(2)                                  // Transmitter/receiver
#define I2C_SR1_GENCALL                     BIT(4)                                  // General call address (Slave mode) 
#define I2C_SR1_SMBDEFAULT                  BIT(5)                                  // SMBus device default address (Slave mode) 
#define I2C_SR1_SMBHOST                     BIT(6)                                  // SMBus host header (Slave mode)
#define I2C_SR1_DUALF                       BIT(7)                                  // Dual flag (Slave mode)

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
void i2c_bitbang_fifo(void);
void i2c_sensor_init(void);
void i2c_bitbang_temp(void);
void debug_gpio_state(void);

// ===== HARDWARE I2C FUNCTIONS =====
void i2c_hardware_init(uint32_t speed_hz);
bool i2c_hardware_write(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len);
bool i2c_hardware_read(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len);
bool i2c_hardware_probe(uint8_t addr);

extern const I2C_config_t MAX30102_I2C_CFG;

#endif // INC_I2C_H