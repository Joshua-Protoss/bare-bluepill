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
    }
}
uint8_t spi_transfer_byte(volatile SPI_reg_t *spi, uint8_t data);
void spi_transfer_buffer(volatile SPI_reg_t *spi, uint8_t *tx_data, uint8_t *rx_data, uint16_t len);
void spi_cs_select(void);
void spi_cs_deselect(void);

const spi_config_t SPI_SD_CARD_TEST = {
    .baud_rate = SPI_BR_DIV_4,                          // 22MHz/4 = 5.5MHz
    .mode = SPI_MODE_0,                                 // Most common mode
    .master = true,
    .msb_first = true,
    .software_cs = true,
    .cs_pin = PIN_GPIO4,
    .cs_port = PORT_GPIOA,
};

