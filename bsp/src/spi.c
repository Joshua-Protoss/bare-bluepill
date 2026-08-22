#include "spi.h"
#include "rcc.h"

static spi_config_t current_spi_config;
static bool spi_initialized = false;

void spi_init(volatile SPI_reg_t *spi, const spi_config_t *config) {
    current_spi_config = *config;

    // Enable SPI clock
    if (spi == SPI1) {
        rcc_periph_clock_enable(RCC_SPI1);
        rcc_periph_clock_enable(RCC_GPIOA);

        // Configure SPI1 pins: PA5=SCK, PA6=MISO, PA7=MOSI
        gpio_set_mode(PORT_GPIOA, PIN_GPIO5, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
        gpio_set_mode(PORT_GPIOA, PIN_GPIO6, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOATING);
        gpio_set_mode(PORT_GPIOA, PIN_GPIO7, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
    } else if (spi == SPI2) {
        rcc_periph_clock_enable(RCC_SPI2);
        rcc_periph_clock_enable(RCC_GPIOB);

        // Configure SPI2 pins: PB13=SCK, PB14=MISO, PB15=MOSI
        gpio_set_mode(PORT_GPIOB, PIN_GPIO13, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
        gpio_set_mode(PORT_GPIOB, PIN_GPIO14, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOATING);
        gpio_set_mode(PORT_GPIOB, PIN_GPIO15, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
    }

    // Configure CS pin as GPIO output
    if (config->software_cs) {
        gpio_set_mode(config->cs_port, config->cs_pin, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_PUSHPULL);
        gpio_write_pin(config->cs_port, config->cs_pin, 1);                                                     // CS HIGH (deselected)
    }

    spi->CR1 = 0x0000;                                                                                          // Reset SPI
    spi->CR2 = 0x0000;                                                                                          // clear CR2
    for (volatile int i = 0; i < 100; i++);                                                                     // Small delay
    uint32_t cr1 = 0;                                                                                           // Configure CR1

    if (config->master) {                                                                                       // Master mode
        cr1 |= SPI_CR1_MSTR;
    }

    cr1 |= config->baud_rate;                                                                                   // Baud rate
    if (config->mode & 0x01) {                                                                                  // CPHA
        cr1 |= SPI_CR1_CPHA;
    }

    if (config->mode & 0x02) {                                                                                  // CPOL
        cr1 |= SPI_CR1_CPOL;
    }

    if (!config->msb_first) {                                                                                   // MSB first
        cr1 |= SPI_CR1_LSBFIRST;
    }

    if (config->software_cs) {                                                                                  // Software slave management
        cr1 |= SPI_CR1_SSM | SPI_CR1_SSI;                                                                       // Software CS + internal slave select
    }

    spi->CR1 = cr1;
    spi->CR2 = 0x0000;                                                                                          // clear CR2
    spi->CR1 |= SPI_CR1_SPE;                                                                                    // Enable SPI
    spi_initialized = true;
}

uint8_t spi_transfer_byte(volatile SPI_reg_t *spi, uint8_t data){
    while(!(spi->SR & SPI_SR_TXE));                                         // Wait for TX buffer empty
    spi->DR = data;                                                         // Send data
    while(!(spi->SR & SPI_SR_RXNE));                                        // Wait for RX buffer not empty
    return (uint8_t)spi->DR;                                                // Read received data
}

void spi_transfer_buffer(volatile SPI_reg_t *spi, uint8_t *tx_data, uint8_t *rx_data, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        uint8_t tx_byte = (tx_data != 0) ? tx_data[i] : 0xFF;
        uint8_t rx_byte = spi_transfer_byte(spi, tx_byte);

        if (rx_data != 0) {
            rx_data[i] = rx_byte;
        }
    }
}

void spi_cs_select(void) {
    if (current_spi_config.software_cs) {
        gpio_write_pin(current_spi_config.cs_port, current_spi_config.cs_pin, 0);
    }
}

void spi_cs_deselect(void) {
    if (current_spi_config.software_cs) {
        gpio_write_pin(current_spi_config.cs_port, current_spi_config.cs_pin, 1);
    }
}

const spi_config_t SPI_SD_CARD_TEST = {
    .baud_rate = SPI_BR_DIV_4,                          // 22MHz/4 = 5.5MHz
    .mode = SPI_MODE_0,                                 // Most common mode
    .master = true,
    .msb_first = true,
    .software_cs = true,
    .cs_pin = PIN_GPIO4,
    .cs_port = PORT_GPIOA,
};

