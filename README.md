# bare-bluepill

**Bare metal library for STM32F103C8T6 Bluepill - No HAL, no CMSIS, just pure embedded C.**

## About
`bare-bluepill` is a complete bare-metal firmware framework written entirely from scratch. Every register, every bit, and every driver is hand-crafted to understand and control the hardware at the lowest level. Zero external dependencies. I might add more peripherals and more complicated projects in the future! 🚀

## Features

### Clock System (RCC)
- ✅ HSI (8MHz internal), HSE (8MHz external crystal)
- ✅ PLL configuration up to 72MHz with auto flash latency
- ✅ Custom clock presets: 8MHz, 44MHz, 72MHz
- ✅ Bus prescalers: AHB, APB1, APB2
- ✅ Clock switching with timeout fallback to HSI
- ✅ Runtime frequency queries

### GPIO Driver
- ✅ Struct-based register access with clean API
- ✅ All ports: GPIOA through GPIOG
- ✅ All modes: Input, Output (10/2/50MHz), Analog
- ✅ Output types: Push-pull, Open-drain, Alternate Function
- ✅ Input configurations: Floating, Pull-up, Pull-down
- ✅ Atomic bit operations via BSRR/BRR registers
- ✅ Mask-based multi-pin configuration

### SysTick Timer
- ✅ Polling mode with COUNTFLAG
- ✅ Interrupt mode with tick counter
- ✅ Auto clock source negotiation (AHB or AHB/8)
- ✅ Frequency configuration in Hz
- ✅ WFI sleep support for low power

### NVIC (Interrupt Controller)
- ✅ Enable/disable individual IRQs
- ✅ Priority configuration (4-bit, 16 levels)
- ✅ All 43 IRQ channels defined for STM32F103

### Timer Driver (TIM2, TIM3, TIM4)
- ✅ PWM output on all 4 channels
- ✅ Output Compare modes: Frozen, Active, Inactive, Toggle, Force, PWM1, PWM2
- ✅ Edge-aligned and Center-aligned PWM
- ✅ Continuous and One-Shot modes
- ✅ Clock division and direction control
- ✅ Auto PSC + ARR calculation from desired frequency
- ✅ Runtime duty cycle updates
- ✅ Named PWM configurations

### USART Driver
- ✅ Polling TX/RX with blocking functions
- ✅ Interrupt-driven RX with line buffering
- ✅ DMA-driven RX with circular buffer
- ✅ Configurable: baud rate, data bits, parity, stop bits
- ✅ Auto baud rate calculation from APB clock
- ✅ Character echo with backspace support
- ✅ String and raw buffer transmit
- ✅ Named USART configurations

### DMA Driver
- ✅ Channel configuration: peripheral→memory, memory→memory
- ✅ Circular mode with half-transfer and full-transfer interrupts
- ✅ Automatic NVIC enable/disable per channel
- ✅ Channel-specific ISR/IFCR flag macros
- ✅ 8/16/32-bit transfer sizes
- ✅ All 7 DMA1 channels mapped to peripherals

What's Next?
ADC driver

I2C driver

SPI driver

Input capture / encoder mode for timers

printf() support via UART

Bootloader with firmware signing

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

