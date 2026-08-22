#include "common.h"
#include "rcc.h"
#include "gpio.h"
#include "systick.h"
#include <string.h>
#include "usart.h"
#include "timers.h"
#include "sdcard.h"

#define SYSTICK_FREQ                    (1000)            // the desired systick frequency, 1000Hz means 1ms per tick  
#define USART_PORT                      (RCC_GPIOA)
#define USART_TX_PIN                    (PIN_GPIO9)       // PA9
#define USART_RX_PIN                    (PIN_GPIO10)      // PA10

// Test: Generate PWM on TIM2 CH1 (PA0), read it with TIM3 CH1 (PA6)
// Connect PA0 to PA6 with a jumper wire!

volatile uint32_t systick_ticks = 0;

// Pre-defined messages as uint8_t arrays
static const uint8_t msg_welcome[] = "\r\n=== SPI SDCARD Test ===\r\n";
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

    volatile uint32_t apb2enr = *(volatile uint32_t*)(PERIPHERAL_APB2_BASE + 0x18);
    // startup messages
    usart_write(USART1, msg_welcome, sizeof(msg_welcome)-1);
    usart_write(USART1, msg_prompt, sizeof(msg_prompt)-1);
    usart_printf(USART1, "SysClk: %lu Hz\r\n", rcc_get_sysclk_freq());
    usart_printf(USART1, "APB1: %lu Hz\r\n", rcc_get_apb1_freq());
    usart_printf(USART1, "APB2: %lu Hz\r\n", rcc_get_apb2_freq());
    usart_printf(USART1, "APB2ENR: 0x%08X\r\n", apb2enr);
    usart_printf(USART1, "GPIOB EN bit: %d\r\n", (apb2enr >> 3) & 1);
}

int main(void) {
    rcc_clock_configure(&RCC_CLOCK_HSE_44MHZ);
    uart_setup();
    usart_printf(USART1, "APB2ENR after clock config: 0x%08X\r\n", RCC->APB2ENR);
    systick_set_frequency(SYSTICK_FREQ, rcc_get_ahb_freq());        // 1ms tick, interrupt enabled by default
    rcc_periph_clock_enable(RCC_GPIOC);
    usart_printf(USART1, "APB2ENR after GPIOC: 0x%08X\r\n", RCC->APB2ENR);
    rcc_periph_clock_enable(RCC_GPIOB);
    usart_printf(USART1, "APB2ENR after GPIOB: 0x%08X\r\n", RCC->APB2ENR);
    gpio_set_mode(PORT_GPIOC, PIN_GPIO13, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_PUSHPULL);
    gpio_set_mode(PORT_GPIOB, PIN_GPIO0, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_PUSHPULL);
    usart_printf(USART1, "GPIOB->CRL: 0x%08X\r\n", PORT_GPIOB->CRL);
    usart_printf(USART1, "GPIOB->ODR: 0x%08X\r\n", PORT_GPIOB->ODR);

    // Test CS
    gpio_write_pin(PORT_GPIOB, PIN_GPIO0, 0);
    usart_printf(USART1, "CS LOW: %d\r\n", gpio_read_pin(PORT_GPIOB, PIN_GPIO0));
    gpio_write_pin(PORT_GPIOB, PIN_GPIO0, 1);
    usart_printf(USART1, "After set: ODR=0x%08X, IDR=0x%08X\r\n", PORT_GPIOB->ODR, PORT_GPIOB->IDR);
    usart_printf(USART1, "CS HIGH: %d\r\n", gpio_read_pin(PORT_GPIOB, PIN_GPIO0));
    sd_card_spi_test();

    while(1){
        systick_delay_ms(200);

        __asm__("wfi");  // Sleep, save power!
    }
       

    return 0;
}

