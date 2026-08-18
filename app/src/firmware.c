#include "common.h"
#include "rcc.h"
#include "gpio.h"
#include "systick.h"
#include <string.h>
#include "usart.h"
#include "timers.h"
#include "nvic.h"
#include "i2c.h"

#define SYSTICK_FREQ                    (1000)            // the desired systick frequency, 1000Hz means 1ms per tick  
#define USART_PORT                      (RCC_GPIOA)
#define USART_TX_PIN                    (PIN_GPIO9)       // PA9
#define USART_RX_PIN                    (PIN_GPIO10)      // PA10

// Test: Generate PWM on TIM2 CH1 (PA0), read it with TIM3 CH1 (PA6)
// Connect PA0 to PA6 with a jumper wire!

volatile uint32_t systick_ticks = 0;

// Pre-defined messages as uint8_t arrays
static const uint8_t msg_welcome[] = "\r\n=== I2C MAX30102 Test ===\r\n";
static const uint8_t msg_prompt[] = "\r\n>";

void systick_handler(void){
    systick_ticks++;
}

void uart_setup(){
    rcc_periph_clock_enable(USART_PORT);
    rcc_periph_clock_enable(RCC_USART1);
    // PA9 = TX (AF push-pull)
    gpio_set_mode(PORT_GPIOA, USART_TX_PIN, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
    // PA10 = RX (floating input)
    gpio_set_mode(PORT_GPIOA, USART_RX_PIN, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOATING);

    // Configure USART1
    usart_init(USART1, 115200, &USART1_TX_RX_8BIT);    // the function will automatically compute the correct BRR for 44MHz

    // startup messages
    usart_write(USART1, msg_welcome, sizeof(msg_welcome)-1);
    usart_write(USART1, msg_prompt, sizeof(msg_prompt)-1);
    usart_printf(USART1, "SysClk: %lu Hz\r\n", rcc_get_sysclk_freq());
    usart_printf(USART1, "APB2: %lu Hz\r\n", rcc_get_apb2_freq());
}

void max30102_i2c_test(void) {
    i2c_bitbang_init(&MAX30102_I2C_CFG);

    // Probe the device
    usart_printf(USART1, "Probing 0x57...");
    if (i2c_probe(0x57)) {
        usart_printf(USART1, "Device found!\r\n");
    } else {
        usart_printf(USART1, "No response. Check wiring. \r\n");
        return;
    }

    // Read Part ID (register 0xFF)
    uint8_t part_id;
    if (i2c_read(0x57, 0xFF, &part_id, 1)) {                                         // "Set your register pointer to 0xFF (Part ID register)"
        usart_printf(USART1, "Part ID: 0x%02X ", part_id);                           // we will get data from the 0xFF (part id) register
        if (part_id == 0x15) {
            usart_printf(USART1, "MAX30102 detected! \r\n");
        } else {
            usart_printf(USART1, "Unknown, expected 0x15 \r\n");
        }
    } else {
        usart_printf(USART1, "Failed to read Part ID \r\n");
    }

    // Read Revision ID (register 0xFE)
    uint8_t rev_id;
    if (i2c_read(0x57, 0xFE, &rev_id, 1)) {
        usart_printf(USART1, "Revision ID: 0x%02X\r\n", rev_id);
    } else {
        usart_printf(USART1, "Failed to read Revision ID \r\n");
    }

    usart_printf(USART1, "=== Test Complete ===\r\n");
}

void i2c_scan_bus(void) {
    usart_printf(USART1, "\r\n=== I2C Bus Scan ===\r\n");
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        if (i2c_probe(addr)) {
            usart_printf(USART1, "Device found at 0x%02X!\r\n", addr);
        }
    }
    usart_printf(USART1, "=== Scan Complete ===\r\n");
}

void i2c_bitbang_temp(void) {
    uint8_t temp_int, temp_frac;

    // Read temperature (integer part)
    if (i2c_read(0x57, 0x1F, &temp_int, 1)) {
        int8_t temp_c = (int8_t) temp_int;
        usart_printf(USART1, "Die Temperature: %d°C\r\n", temp_c);
    }

    // Read temperature fractional (0x20)
    if (i2c_read(0x57, 0x20, &temp_frac, 1)) {
        float temp = (int8_t)temp_int + (temp_frac * 0.0625f);
        usart_printf(USART1, "Precise Temp: %.2f°C\r\n", temp);
    }

    // Read interrupt status (0x00)
    uint8_t int_status;
    if (i2c_read(0x57, 0x00, &int_status, 1)) {
        usart_printf(USART1, "Interrupt Status: 0x%02X\r\n", int_status);
    }
}

void i2c_sensor_init(void) {
    // Reset the device
    i2c_write(0x57, 0x09, (uint8_t[]){0x40}, 1);            // MODE: Reset
    systick_delay_ms(100);                                  // Wait for reset
    // FIFO Configuration
    i2c_write(0x57, 0x08, (uint8_t[]){0x4F}, 1);            // Sample avg=4, FIFO rollover
    // Mode Configuration: SpO2 mode
    i2c_write(0x57, 0x09, (uint8_t[]){0x03}, 1);            // SpO2 mode
    // SpO2 Configuration
    i2c_write(0x57, 0x0A, (uint8_t[]){0x27}, 1);            // ADC range = 4096nA, sample rate=100Hz, pulse width=411µs
    // LED Pulse Amplitude (IR LED)                         // if your raw values are already above 100000 leave it
    i2c_write(0x57, 0x0D, (uint8_t[]){0x24}, 1);            // ~7.2mA, if your raw values are sitting below 50,000 increase it
    // LED Pulse Amplitude (Red LED)                        
    i2c_write(0x57, 0x0C, (uint8_t[]){0x24}, 1);            // ~7.2mA
    // Enable temperature sensor
    i2c_write(0x57, 0x21, (uint8_t[]){0x01}, 1);
    //  Wait for everything to stabilize
    systick_delay_ms(50);
}

// Read FIFO data (3 bytes per sample: IR[18:11], IR[10:3], IR[2:0]+Red[18:15], etc.)
void i2c_bitbang_fifo(void) {
    uint8_t fifo_data[6];                                   // 2 samples worth
    // Read 6 bytes from FIFO (0x07)
    if (i2c_read(0x57, 0x07, fifo_data, 6)) {
        // Parse first sample (first 3 bytes)
        uint32_t ir_sample  = (fifo_data[0] << 16) | (fifo_data[1] << 8) | fifo_data[2];
        uint32_t red_sample = (fifo_data[3] << 16) | (fifo_data[4] << 8) | fifo_data[5];

        ir_sample &= 0x3FFFF;               // 18-bit value
        red_sample &= 0x3FFFF;              // 18-bit value

        usart_printf(USART1, "IR: %lu, Red: %lu\r\n", ir_sample, red_sample);
    }
}

int main(void) {
    rcc_clock_configure(&RCC_CLOCK_HSE_44MHZ);
    systick_set_frequency(SYSTICK_FREQ, rcc_get_ahb_freq());        // 1ms tick, interrupt enabled by default
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOB);
    gpio_set_mode(PORT_GPIOC, PIN_GPIO13, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_PUSHPULL);
    uart_setup();
    
    usart_printf(USART1, "APB1 Timer Clock: %lu Hz\r\n", rcc_get_apb1_timer_freq());
    usart_printf(USART1, "Expected PWM period: %lu ticks @ 1kHz\r\n", rcc_get_apb1_timer_freq() / 1000);
    max30102_i2c_test();
    i2c_scan_bus();
    i2c_sensor_init();

    while(1){
        i2c_bitbang_temp();
        i2c_bitbang_fifo();
        systick_delay_ms(20);
        __asm__("wfi");  // Sleep, save power!
    }
       

    return 0;
}

