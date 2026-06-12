#include "common.h"
#include "rcc.h"
#include "gpio.h"
#include "systick.h"

#define LED_PORT         (PORT_GPIOC)
#define LED_PIN          (PIN_GPIO13)

volatile uint32_t systick_ticks = 0;

void gpio_setup(void){
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_set_mode(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_PUSHPULL);
}

void systick_handler(void){
    systick_ticks++;
}

int main(void) {
    rcc_clock_configure(&RCC_CLOCK_HSE_44MHZ);
    gpio_setup();
    systick_set_frequency(1000, rcc_get_ahb_freq()); // 1ms tick

    // PWM parameters
    uint32_t pwm_period = 20;   // 10ms period (100Hz)
    uint32_t duty_cycle = 0;    // 0-10 (0% to 100%)
    int8_t direction = 1;
    //uint32_t last_tick = 0;
    //uint32_t on_time;
    bool led_is_on = false;
    uint32_t cycle_start = 0;

    while(1){

        uint32_t now = systick_ticks;
        
        // Start of new PWM cycle
        if (now - cycle_start >= pwm_period) {
            cycle_start = now;
            duty_cycle += direction;
            
            // Reverse direction at limits
            if (duty_cycle >= pwm_period) {
                direction = -1;
            } else if (duty_cycle == 0) {
                direction = 1;
            }
        }

        // PWM control
        uint32_t cycle_time = now - cycle_start;
        
        if (cycle_time < duty_cycle) {
            if (!led_is_on) {
                gpio_write_pin(LED_PORT, LED_PIN, false);  // ON (active low)
                led_is_on = true;
            }
        } else {
            if (led_is_on) {
                gpio_write_pin(LED_PORT, LED_PIN, true);   // OFF
                led_is_on = false;
            }
        }
        
        __asm__("wfi");
    }
    return 0;
}