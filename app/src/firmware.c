#include "common.h"
#include "rcc.h"
#include "gpio.h"

#define LED_PORT         (PORT_GPIOC)
#define LED_PIN          (PIN_GPIO13)

static void scuffed_delay(uint32_t count) {
    volatile uint32_t i;
    for (i = 0; i < count; i++){
        __asm__("nop");
    }
} 

int main(void) {
    rcc_clock_configure(&RCC_CLOCK_HSE_44MHZ);
    uint32_t sysclk = rcc_get_sysclk_freq();
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_set_mode(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_PUSHPULL);

    while(1){
        gpio_toggle_pin(PORT_GPIOC, PIN_GPIO13);
        scuffed_delay(sysclk / 44);
    }
    return 0;
}