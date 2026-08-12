#include "dac.h"
#include "rcc.h"
#include "gpio.h"

// Simple initialization
void dac_init(void) {
    rcc_periph_clock_enable(RCC_DAC);
    
    // Configure PA4 as analog (DAC_OUT1)
    gpio_set_mode(PORT_GPIOA, PIN_GPIO4, GPIO_MODE_INPUT,GPIO_CNF_INPUT_ANALOG);

    // Optional: PA5 = DAC_OUT2
    // gpio_set_mode(PORT_GPIOA, PIN_GPIO5, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG);
    
    // Enable DAC channel 1
    DAC->CR |= BIT(0);  // DAC_CH1_EN

    // Channel 1 configuration:
    // Bit 0: EN1 = 1 (enable channel 1)
    // Bit 1: BOFF1 = 0 (no buffer calibration, or 1 for factory trim)
    // Bit 2: TEN1 = 0 (trigger disabled, or 1 for timer-triggered update)
    // Bits 3-5: TSEL1 = 000 (timer trigger selection, if TEN1=1)
    // Bit 6: WAVE1 = 0 (no noise/triangle wave generation)
    // Bits 7-8: MAMP1 = 00 (wave amplitude, if WAVE1=1)
    // Bit 12: DMAEN1 = 0 (DMA disabled)
}

// Set output voltage (0-4095 → 0-Vref)
void dac_set(uint16_t value) {
    DAC->DHR12R1 = value & 0xFFF;
}

void dac_init_triggered(void) {
    rcc_periph_clock_enable(RCC_DAC);
    gpio_set_mode(PORT_GPIOA, PIN_GPIO4, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG);
    
    // Enable DAC with TIM2 TRGO as trigger
    DAC->CR = BIT(0);           // EN1 = 1
    DAC->CR |= BIT(2);          // TEN1 = 1 (trigger enabled)
    DAC->CR |= (0x04 << 3);     // TSEL1 = 100 (TIM2 TRGO)
    
    // Now DAC updates DHR12R1 → DOR1 only on TIM2 trigger events
    // This gives you glitch-free updates synchronized with PWM
}

// STEPS :
// 1. Initialize
//dac_init();

// 2. Set output to 1.65V (half of 3.3V)
//dac_set(2048);  // 4096/2 = 2048

// 3. Ramp test
// for (uint16_t i = 0; i < 4096; i += 16) {
//     dac_set(i);
//     systick_delay_ms(1);
// }

// 4. Verify with your input capture or ADC!