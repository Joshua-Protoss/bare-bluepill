#ifndef INC_SPI_H
#define INC_SPI_H

#include "common.h"
#include "gpio.h"

// SPI base addresses
#define SPI1_BASE                       (PERIPHERAL_APB2_BASE + 0x3000U)
#define SPI2_BASE                       (PERIPHERAL_APB1_BASE + 0x3800U)
#define SPI3_BASE                       (PERIPHERAL_APB1_BASE + 0x3C00U)                // not available in bluepill

// SPI register struct
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t CRCPR;
    volatile uint32_t RXCRCR;
    volatile uint32_t TXCRCR;
    volatile uint32_t I2SCFGR;
    volatile uint32_t I2SPR;
} SPI_reg_t;

// SPI Instances
#define SPI1                            ((volatile SPI_reg_t *) SPI1_BASE)
#define SPI2                            ((volatile SPI_reg_t *) SPI2_BASE)
#define SPI3                            ((volatile SPI_reg_t *) SPI3_BASE)

// ===== CR1 Bits =====
#define SPI_CR1_CPHA                    BIT(0)
#define SPI_CR1_CPOL                    BIT(1)
#define SPI_CR1_MSTR                    BIT(2)
#define SPI_CR1_BR_MASK                 (0x07 << 3)
#define SPI_CR1_SPE                     BIT(6)
#define SPI_CR1_LSBFIRST                BIT(7)
#define SPI_CR1_SSI                     BIT(8)
#define SPI_CR1_SSM                     BIT(9)
#define SPI_CR1_RXONLY                  BIT(10)
#define SPI_CR1_DFF                     BIT(11)
#define SPI_CR1_CRCNEXT                 BIT(12)
#define SPI_CR1_CRCEN                   BIT(13)
#define SPI_CR1_BIDIOE                  BIT(14)
#define SPI_CR1_BIDIMODE                BIT(15)

// baud rate control
typedef enum {
    SPI_BR_DIV_2     = (0x00 << 3),
    SPI_BR_DIV_4     = (0x01 << 3),
    SPI_BR_DIV_8     = (0x02 << 3),
    SPI_BR_DIV_16    = (0x03 << 3),
    SPI_BR_DIV_32    = (0x04 << 3),
    SPI_BR_DIV_64    = (0x05 << 3),
    SPI_BR_DIV_128   = (0x06 << 3),
    SPI_BR_DIV_256   = (0x07 << 3),
} spi_br_mode_t;

// ===== CR2 Bits =====
#define SPI_CR2_RXDMAEN                BIT(0)
#define SPI_CR2_TXDMAEN                BIT(1)
#define SPI_CR2_SSOE                   BIT(2)                       // Slave Select Output Enable
#define SPI_CR2_ERRIE                  BIT(5)                       // Error interrupt enable
#define SPI_CR2_RXNEIE                 BIT(6)                       // RX buffer not empty interrupt enable
#define SPI_CR2_TXEIE                  BIT(7)                       // Tx buffer empty interrupt enable

// ===== SR Bits =====
#define SPI_SR_RXNE                    BIT(0)                       // Receive buffer not empty
#define SPI_SR_TXE                     BIT(1)                       // Transmit buffer empty
#define SPI_SR_CHSIDE                  BIT(2)                       // Channel side
#define SPI_SR_UDR                     BIT(3)                       // Underrun flag
#define SPI_SR_CRCERR                  BIT(4)                       // CRC error flag
#define SPI_SR_MODF                    BIT(5)                       // Mode fault
#define SPI_SR_OVR                     BIT(6)                       // Overrun flag
#define SPI_SR_BSY                     BIT(7)                       // Busy flag

// ===== I2SCFGR Bits =====
#define I2S_CFGR_CHLEN                 BIT(0)                       // Channel length (number of bits per audio channel)
#define I2S_CFGR_DATLEN_MASK           (0x03 << 2)                  // Data length to be transferred
#define I2S_CFGR_CKPOL                 BIT(3)                       // Steady state clock polarity
#define I2S_CFGR_I2SSTD_MASK           (0x03 << 4)                  // I2S standard selection
#define I2S_CFGR_PCMSYNC               BIT(7)                       // PCM frame synchronization
#define I2S_CFGR_I2SCFG_MASK           (0x03 << 8)                  // I2S configuration mode
#define I2S_CFGR_I2SE                  BIT(10)                      // I2S Enable
#define I2S_CFGR_I2SMOD                BIT(11)                      // I2S mode selection

// ===== I2SPR Bits =====
#define I2S_SPR_ODD                    BIT(8)                       // Odd factor for the prescaler
#define I2S_SPR_MCKOE                  BIT(9)                       // Master clock output enable

// SPI Modes struct (CPOL, CPHA)
typedef enum {
    SPI_MODE_0 = 0x00,                                  // CPOL=0, CPHA=0 (idle LOW, sample on rising)
    SPI_MODE_1 = 0x01,                                  // CPOL=0, CPHA=1 (idle LOW, sample on falling)
    SPI_MODE_2 = 0x02,                                  // CPOL=1, CPHA=0 (idle HIGH, sample on falling)
    SPI_MODE_3 = 0x03,                                  // CPOL=1, CPHA=1 (idle HIGH, sample on rising)
} spi_mode_t;

// Configuration struct
typedef struct {
    spi_br_mode_t baud_rate;                            // Clock divider
    spi_mode_t mode;                                    // CPOL/CPHA
    bool master;                                        // Master mode
    bool msb_first;                                     // MSB first (vs LSB first)
    bool software_cs;                                   // Software CS (GPIO)
    uint8_t cs_pin;                                     // CS GPIO pin
    GPIO_reg_t *cs_port;                                // CS GPIO port
} spi_config_t;

// Function prototypes
void spi_init(volatile SPI_reg_t *spi, const spi_config_t *config);
uint8_t spi_transfer_byte(volatile SPI_reg_t *spi, uint8_t data);
void spi_transfer_buffer(volatile SPI_reg_t *spi, uint8_t *tx_data, uint8_t *rx_data, uint16_t len);
void spi_cs_select(void);
void spi_cs_deselect(void);

// Bitbang function prototypes
uint8_t spi_bitbang_read(uint8_t reg_addr);
uint8_t spi_bitbang_write(uint8_t data_out);

#endif // INC_SPI_H