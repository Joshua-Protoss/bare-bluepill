# bare-bluepill

**Bare metal library for STM32F103C8T6 Bluepill - No HAL, no CMSIS, just pure embedded C.**

## About
`bare-bluepill` is a from-scratch implementation of peripheral drivers for the STM32F103C8T6 Bluepill board. Written entirely without vendor libraries to understand and control every single bit. I might add more peripherals and more complicated projects in the future! 🚀

## Current Status
✅ GPIO Driver (Output, Push-pull)  
✅ RCC Clock Control  
✅ Startup Code & Vector Table  
⏳ Input Mode & Pull-up/down  
⏳ UART  
⏳ Timers  
⏳ SysTick  

Demo video here: https://youtu.be/WjqHCQbmScI

## Quick Example
```c
#include "rcc.h"
#include "gpio.h"

int main(void) {
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_set_mode(PORT_GPIOC, PIN_GPIO13, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_PUSHPULL);
    
    while(1) {
        gpio_write_pin(PORT_GPIOC, PIN_GPIO13, false);  // LED on
        scuffed_delay(8000000);
        gpio_write_pin(PORT_GPIOC, PIN_GPIO13, true);   // LED off
        scuffed_delay(8000000);
    }
}
