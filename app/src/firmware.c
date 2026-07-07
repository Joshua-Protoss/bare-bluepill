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

static volatile uint8_t rx_buffer[RX_BUFF_SIZE];
static volatile uint8_t rx_index = 0;

// Pre-defined messages as uint8_t arrays
static const uint8_t msg_welcome[] = "Blue Pill Echo Terminal\r\n";
static const uint8_t msg_prompt[]  = "Type something and press Enter:\r\n";
static const uint8_t msg_prefix[]  = "\r\nYou typed: ";
static const uint8_t msg_suffix[]  = "\r\n> ";
static const uint8_t msg_newline[] = "\r\n";
static const uint8_t msg_prompt2[] = "> ";

static void process_line(const volatile uint8_t *line, uint8_t length) {
    usart_write(USART1, msg_prefix, sizeof(msg_prefix)-1);
    usart_write(USART1, (const uint8_t*)line, length);
    usart_write(USART1, msg_suffix, sizeof(msg_suffix)-1);  // New prompt
}

void gpio_setup(void){
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_TIM2);
    gpio_set_mode(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
    gpio_set_mode(LED2_PORT, LED2_PIN, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
}

void systick_handler(void){
    systick_ticks++;
}

void usart1_isr(void) {
    // === UART Echo ===
    if (USART1->SR & USART_SR_RXNE) {
        uint8_t c = (uint8_t)(USART1->DR & 0xFF);

        if (c == '\r' || c == '\n'){    // Enter pressed
            if (rx_index > 0) {
                process_line(rx_buffer, rx_index);
            } else {
                usart_write(USART1, msg_newline, sizeof(msg_newline)-1);
                usart_write(USART1, msg_prompt2, sizeof(msg_prompt2)-1);
            }
            rx_index = 0;
        } else if (c == 8 || c == 127) {    // Backspace
            if (rx_index > 0) {
                rx_index--;
                usart_write_DR(USART1, '\b');
                usart_write_DR(USART1, ' ');
                usart_write_DR(USART1, '\b');
            }

        } else if (rx_index < RX_BUFF_SIZE - 1) {   // normal character --> save to buffer
            rx_buffer[rx_index++] = c;
            usart_write_DR(USART1, c);              // echo the character immediately
        }
    }
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
    usart_rx_interrupt_enable(USART1);

    // startup messages
    for (volatile uint32_t i = 0; i < 1000000; i++);
    usart_write(USART1, msg_welcome, sizeof(msg_welcome)-1);
    usart_write(USART1, msg_prompt, sizeof(msg_prompt)-1);
    usart_write(USART1, msg_prompt2, sizeof(msg_prompt2)-1);
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

    while(1){

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
        
       __asm__("wfi");  // Sleep, save power!
    }
    return 0;
}