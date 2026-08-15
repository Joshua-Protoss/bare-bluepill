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

void debug_gpio_state(void) {
    usart_printf(USART1, "\r\n=== GPIO Debug ===\r\n");
    
    // Set SCL high, read back
    scl_high();
    usart_printf(USART1, "SCL after high: %d\r\n", 
                 gpio_read_pin(PORT_GPIOB, PIN_GPIO6));
    
    // Set SDA high, read back
    sda_high();
    usart_printf(USART1, "SDA after high: %d\r\n", 
                 gpio_read_pin(PORT_GPIOB, PIN_GPIO7));
    
    // Set SDA low, read back
    sda_low();
    usart_printf(USART1, "SDA after low: %d\r\n", 
                 gpio_read_pin(PORT_GPIOB, PIN_GPIO7));
    sda_high();
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

int main(void) {
    rcc_clock_configure(&RCC_CLOCK_HSE_44MHZ);
    systick_set_frequency(SYSTICK_FREQ, rcc_get_ahb_freq());        // 1ms tick, interrupt enabled by default
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_set_mode(PORT_GPIOC, PIN_GPIO13, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_PUSHPULL);
    uart_setup();
    
    usart_printf(USART1, "APB1 Timer Clock: %lu Hz\r\n", rcc_get_apb1_timer_freq());
    usart_printf(USART1, "Expected PWM period: %lu ticks @ 1kHz\r\n", rcc_get_apb1_timer_freq() / 1000);
    max30102_i2c_test();

    while(1){
        __asm__("wfi");  // Sleep, save power!

    }
       

    return 0;
}

