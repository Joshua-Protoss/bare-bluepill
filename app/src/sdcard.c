#include "sdcard.h"
#include "usart.h"
#include "systick.h"

void debug_cs_pin(void) {
    // Test CS pin
    gpio_write_pin(PORT_GPIOB, PIN_GPIO0, 0);
    usart_printf(USART1, "CS LOW: %d\r\n", gpio_read_pin(PORT_GPIOB, PIN_GPIO0));
    systick_delay_ms(100);
    gpio_write_pin(PORT_GPIOB, PIN_GPIO0, 1);
    usart_printf(USART1, "CS HIGH: %d\r\n", gpio_read_pin(PORT_GPIOB, PIN_GPIO0));
}

void sd_card_spi_test(void) {
    uint8_t response;
    uint32_t timeout;
    // Configure SPI for SD card
    spi_config_t sd_spi_cfg = {
        .baud_rate = SPI_BR_DIV_128,                        // Slow for init (22MHz/256 = 86kHz)
        .mode = SPI_MODE_0,
        .master = true,
        .msb_first = true,
        .software_cs = true,
        .cs_pin = PIN_GPIO0,
        .cs_port = PORT_GPIOB,
    };
    
    spi_init(SPI1, &sd_spi_cfg);
    usart_printf(USART1, "SPI CR1: 0x%08X\r\n", SPI1->CR1);
    usart_printf(USART1, "SPI CR2: 0x%08X\r\n", SPI1->CR2);
    spi_cs_deselect();
    systick_delay_ms(10);

    for (int i = 0; i < 10; i++) {                                 // Send at least 74 clock pulses (required for SD init)
        spi_transfer_byte(SPI1, 0xFF);
    }
    systick_delay_ms(10);

    spi_cs_select();
    systick_delay_ms(10);

    // Send CMD0 (reset)
    uint8_t cmd0[] = {CMD0, 0x00, 0x00, 0x00, 0x00, 0x95};        // CRC for CMD0
    for (int i = 0; i < 6; i++) {
        spi_transfer_byte(SPI1, cmd0[i]);
    }

    // Read R1 response with timeout
    timeout = 100;
    do {
        response = spi_transfer_byte(SPI1, 0xFF);
        timeout--;
    } while (response == 0xFF && timeout > 0);

    spi_cs_deselect();

    usart_printf(USART1, "CMD0 Response: 0x%02X", response);
    if (response == 0x01) {
        usart_printf(USART1, "(Card in idle state - OK!)\r\n");
    } else if (response == 0x7F) {
        usart_printf(USART1, "(0x7F - Check: VCC=5V? Mode 0? CS correct?)\r\n");
    } else {
        usart_printf(USART1, "(Unexpected response)\r\n");
    }

}