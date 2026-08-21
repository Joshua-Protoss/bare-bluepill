#include "common.h"
#include "rcc.h"
#include "gpio.h"
#include "systick.h"
#include <string.h>
#include "usart.h"
#include "timers.h"

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
    usart_printf(USART1, "APB1: %lu Hz\r\n", rcc_get_apb1_freq());
    usart_printf(USART1, "APB2: %lu Hz\r\n", rcc_get_apb2_freq());
}

int main(void) {
    rcc_clock_configure(&RCC_CLOCK_HSE_44MHZ);
    systick_set_frequency(SYSTICK_FREQ, rcc_get_ahb_freq());        // 1ms tick, interrupt enabled by default
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOB);
    gpio_set_mode(PORT_GPIOC, PIN_GPIO13, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_PUSHPULL);
    uart_setup();
    
    while(1){
        systick_delay_ms(200);

        __asm__("wfi");  // Sleep, save power!
    }
       

    return 0;
}

