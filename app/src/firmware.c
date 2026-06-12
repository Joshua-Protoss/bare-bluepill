#include "common.h"
#include "rcc.h"
#include "gpio.h"
#include "systick.h"
#include "tim.h"

#define LED_PORT         (PORT_GPIOA)       // this is an external LED connected to PA0
#define LED_PIN          (PIN_GPIO0)

volatile uint32_t systick_ticks = 0;

void gpio_setup(void){
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_TIM2);
    gpio_set_mode(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
}

void systick_handler(void){
    systick_ticks++;
}

int main(void) {
    rcc_clock_configure(&RCC_CLOCK_HSE_44MHZ);
    gpio_setup();

    // PWM configuration: 1kHz, 50% duty cycle on CH1
    tim_pwm_config_t pwm_config = {
        .frequency = 1000,
        .duty_cycle = 50,
        .channel = TIM_CH1,
    };

    // TIM2 clock = APB1 frequency × 2
    uint32_t tim_clock = rcc_get_apb1_freq() * 2;
    tim_pwm_init(TIM2, &pwm_config, tim_clock);

    // Sweep duty cycle up and down
    int8_t duty = 0;
    int8_t direction = 1;
    uint32_t last_update = systick_ticks;

    systick_set_frequency(1000, rcc_get_ahb_freq()); // 1ms tick

    while(1){

        // Update duty cycle every 10ms
        if (systick_ticks - last_update >= 10) {
            last_update = systick_ticks;
            
            duty += direction;
            if (duty >= 100) {
                direction = -1;
            } else if (duty <= 0) {
                direction = 1;
            }
            
            tim_pwm_set_duty(TIM2, TIM_CH1, duty);
        }
        
       __asm__("wfi");  // Sleep, save power!
    }
    return 0;
}