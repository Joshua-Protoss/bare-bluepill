#include "common.h"
#include "rcc.h"
#include "gpio.h"
#include "systick.h"
#include <string.h>
#include "usart.h"
#include "timers.h"
#include "nvic.h"

#define SYSTICK_FREQ                    (1000)            // the desired systick frequency, 1000Hz means 1ms per tick  
#define USART_PORT                      (RCC_GPIOA)
#define USART_TX_PIN                    (PIN_GPIO9)       // PA9
#define USART_RX_PIN                    (PIN_GPIO10)      // PA10

// Test: Generate PWM on TIM2 CH1 (PA0), read it with TIM3 CH1 (PA6)
// Connect PA0 to PA6 with a jumper wire!

volatile uint32_t systick_ticks = 0;

// Pre-defined messages as uint8_t arrays
static const uint8_t msg_welcome[] = "Input Capture Terminal\r\n";
static const uint8_t msg_prompt[] = "\r\n>";

void systick_handler(void){
    systick_ticks++;
}

void timer3_isr(void) {
    static uint32_t last_rising = 0;
    static uint32_t period = 0;
    static bool measuring_duty = false;             // false = waiting for rising, true = waiting for falling

    if (TIM3->SR & TIM_SR_CC1IF) {
        uint32_t capture = TIM3->CCR1;

        if (!measuring_duty) {
             // === RISING EDGE CAPTURED ===
             if (capture >= last_rising) {
                period = capture - last_rising;
             } else {
                period = (0xFFFF - last_rising) + capture + 1;
             }

            last_rising = capture;
            // Now switch to falling edge to measure duty
            TIM3->CCER |= TIM_CCER_CC1P;                // Set CC1P=1 → falling edge
            measuring_duty = true;
        } else {
            // === FALLING EDGE CAPTURED ===
            uint32_t duty_ticks;
            if (capture >= last_rising) {
                duty_ticks = capture - last_rising;
            } else {
                duty_ticks = (0xFFFF - last_rising) + capture + 1;
            }

            // Store the measurements
            tim_pwm_capture_t *cap = tim_ic_get_pwm_capture();
            cap->period = period;
            cap->duty = duty_ticks;
            cap->new_data = true;

            // Switch back to rising edge for next cycle
            TIM3->CCER &= ~TIM_CCER_CC1P;               // Set CC1P=0 → rising edge
            measuring_duty = false;
        }
        TIM3->SR &= ~TIM_SR_CC1IF;  
    } 
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

void input_capture_test(void) {
    rcc_periph_clock_enable(RCC_TIM2);              // GPIO port A should be activated in uart_setup()
    rcc_periph_clock_enable(RCC_TIM3);              // make sure uart_setup() is called first

    // PA0 = TIM2_CH1 output (generate PWM)
    gpio_set_mode(PORT_GPIOA, PIN_GPIO0, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
    // PA6 = TIM3_CH1 input (read PWM)
    gpio_set_mode(PORT_GPIOA, PIN_GPIO6, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOATING);

    uint32_t apb1_timer_clk = rcc_get_apb1_timer_freq();  // Should be 44 MHz

    // Generate 1kHz PWM with 30% duty cycle
    tim_oc_init(TIM2, &PWM_CH1_1KHZ_30, apb1_timer_clk);
    // Configure TIM3 CH1 to read PWM in PWM input mode
    tim_ic_init(TIM3, &INPUT_CAPTURE_RISING_44MHZ, apb1_timer_clk);
    // enable nvic
    nvic_enable_irq(NVIC_TIM3_IRQ);
}

int main(void) {
    rcc_clock_configure(&RCC_CLOCK_HSE_44MHZ);
    systick_set_frequency(SYSTICK_FREQ, rcc_get_ahb_freq());        // 1ms tick, interrupt enabled by default
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_set_mode(PORT_GPIOC, PIN_GPIO13, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_PUSHPULL);
    uart_setup();
    input_capture_test();

    usart_printf(USART1, "APB1 Timer Clock: %lu Hz\r\n", rcc_get_apb1_timer_freq());
    usart_printf(USART1, "Expected PWM period: %lu ticks @ 1kHz\r\n", rcc_get_apb1_timer_freq() / 1000);

    while(1){
        if (tim_ic_is_new_data_ready()) {
            tim_ic_clear_new_data();

            tim_pwm_capture_t *cap = tim_ic_get_pwm_capture();
            uint32_t period = cap->period;
            uint32_t duty = cap->duty;

            if (period > 0 && period < 100000) {
                float duty_pct = ((float)duty / (float)period) * 100.0f;
                float freq = (float)rcc_get_apb1_timer_freq() / (float)period;
                usart_printf(USART1, "Period: %lu ticks, Duty: %.1f%%, Freq: %.1fHz \r\n", period, duty_pct, freq);
            }
        }
       //__asm__("wfi");  // Sleep, save power!
    }

    return 0;
}

