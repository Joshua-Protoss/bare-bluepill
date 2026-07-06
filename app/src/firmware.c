#include "common.h"
#include "rcc.h"
#include "gpio.h"
#include "systick.h"
#include "timers.h"
#include "usart.h"

#define LED_PORT         (PORT_GPIOA)       // this is an external LED connected to PA0
#define LED_PIN          (PIN_GPIO0)
#define LED2_PORT        (PORT_GPIOA) 
#define LED2_PIN         (PIN_GPIO1)       // TIM2_CH2

#define RX_BUFF_SIZE        64
volatile uint32_t systick_ticks = 0;

static char rx_buffer[RX_BUFF_SIZE];
static uint8_t rx_index = 0;

void gpio_setup(void){
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_TIM2);
    gpio_set_mode(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
    gpio_set_mode(LED2_PORT, LED2_PIN, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
}

void systick_handler(void){
    systick_ticks++;
}

void uart_setup(){
    // USART Setup
    rcc_periph_clock_enable(RCC_USART1);
    // PA9 = TX (AF push-pull)
    gpio_set_mode(PORT_GPIOA, PIN_GPIO9, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
    // PA10 = RX (floating input)
    gpio_set_mode(PORT_GPIOA, PIN_GPIO10, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOATING);

    // Configure USART1
    usart_init(USART1, 115200, &USART1_TX_RX_8BIT);    // the function will automatically compute the correct BRR for 44MHz

    // startup messages
    for (volatile uint32_t i = 0; i < 1000000; i++);
    usart_write_string(USART1, "Blue Pill Echo Terminal\r\n");
    usart_write_string(USART1, "Type something and press Enter:\r\n");
}

void process_line(const char *line) {
    usart_write_string(USART1, "\r\nYou typed: ");
    usart_write_string(USART1, line);
    usart_write_string(USART1, "\r\n> ");  // New prompt
}

int main(void) {
    rcc_clock_configure(&RCC_CLOCK_HSE_44MHZ);
    gpio_setup();
    systick_set_frequency(1000, rcc_get_ahb_freq()); // 1ms tick
    uart_setup();

    // TIM2 clock = APB1 frequency × 2
    uint32_t tim_clock = rcc_get_apb1_freq() * 2;
    tim_pwm_init(TIM2, &PWM_CH1_1KHZ_50, tim_clock);    // channel 1
    tim_pwm_init(TIM2, &PWM_CH2_1KHZ_50, tim_clock);    // channel 2 PWM

    // Sweep duty cycle up and down
    int8_t duty = 0;
    int8_t direction = 1;
    uint32_t last_update = systick_ticks;

    usart_write_string(USART1, "> ");

    while(1){

        // === UART Echo ===
        if (usart_rx_available(USART1)) {
            char c = (char)usart_read_DR(USART1);
            
            if (c == '\r' || c == '\n') {
                // Enter pressed - line complete
                rx_buffer[rx_index] = '\0';  // Null-terminate
                
                if (rx_index > 0) {
                    process_line(rx_buffer);  // Echo the line back
                } else {
                    usart_write_string(USART1, "\r\n> ");  // Empty line, new prompt
                }
                
                rx_index = 0;  // Reset for next line
                
            } else if (c == 8 || c == 127) {
                // Backspace/Delete - remove last character
                if (rx_index > 0) {
                    rx_index--;
                    usart_write_string(USART1, "\b \b");  // Erase on terminal
                }
                
            } else if (rx_index < RX_BUFF_SIZE - 1) {
                // Normal character - store and echo
                rx_buffer[rx_index++] = c;
                usart_write_DR(USART1, c);  // Echo immediately
            }
        }

        // PWM : Update duty cycle every 10ms
        if (systick_ticks - last_update >= 10) {
            last_update = systick_ticks;
            
            duty += direction;
            if (duty >= 100) {
                direction = -1;
            } else if (duty <= 0) {
                direction = 1;
            }
            
            tim_pwm_set_duty(TIM2, TIM_CH1, duty);
            tim_pwm_set_duty(TIM2, TIM_CH2, 100 - duty);  // LED2: 100→0 (opposite!)

        }
        
        // Send a character
        
       __asm__("wfi");  // Sleep, save power!
    }
    return 0;
}