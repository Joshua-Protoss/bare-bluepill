# bare-bluepill

**Bare metal library for STM32F103C8T6 Bluepill - No HAL, no CMSIS, just pure embedded C.**

## About
`bare-bluepill` is a from-scratch implementation of peripheral drivers for the STM32F103C8T6 Bluepill board. Written entirely without vendor libraries to understand and control every single bit. I might add more peripherals and more complicated projects in the future! 🚀

## Current Status
✅ GPIO Driver (Output, Push-pull)  
✅ RCC Clock Control  
✅ Startup Code & Vector Table  
✅ SysTick timer
✅ General purpose timers for PWM
⏳ Input Mode & Pull-up/down  
⏳ UART  
⏳ SPI, I2C, DMA

Demo video here: https://youtu.be/WjqHCQbmScI

## Quick Example
```c
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

    uint32_t start_time = systick_ticks;

    while(1){

        if (systick_ticks - start_time >= 500) {
            gpio_toggle_pin(LED_PORT, LED_PIN);
            start_time = systick_ticks;
        }

       __asm__("wfi");  // Sleep, save power!
    }
    return 0;
}
