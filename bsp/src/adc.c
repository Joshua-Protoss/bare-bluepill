#include "adc.h"

void adc_init(volatile ADC_reg_t *adc, const ADC_config_t *config){

    // 1. Set ADC clock prescaler (must be ≤ 14 MHz!)
    RCC_CFGR &= ~(0x03 << RCC_CFGR_ADCPRE_SHIFT);          // Clear ADCPRE bits
    RCC_CFGR |= (config->prescaler << RCC_CFGR_ADCPRE_SHIFT);
    
    // 2. Enable ADC clock
    if (adc == ADC1){
        rcc_periph_clock_enable(RCC_ADC1);
    } else if (adc == ADC2) {
        rcc_periph_clock_enable(RCC_ADC2);
    }   // if there is adc3 --> add another else if
    
    // 3. Power on ADC (Wake up the peripheral from power-down mode)
    adc->CR2 |= ADC_CR2_ADON; 

    // Simple delay (no fancy assembly)
    for (volatile uint32_t i = 0; i < 10000; i++);

    // second ADON
    adc->CR2 |= ADC_CR2_ADON;
    for (volatile uint32_t i = 0; i < 10000; i++);

    // 4. Calibrate ADC
    adc->CR2 |= ADC_CR2_RSTCAL;                                         // Reset calibration
    while(adc->CR2 & ADC_CR2_RSTCAL);                                   // Wait for reset calibration complete (hardware clears)
    adc->CR2 |= ADC_CR2_CAL;
    while(adc->CR2 & ADC_CR2_CAL);                                      // Wait for calibration complete

    // 5. Set sample time for the channel
    if (config->channel <= ADC_CH9) {
        // Channels 0-9 use SMPR2
        uint32_t shift = (config->channel) * 3;
        adc->SMPR2 &= ~(0x07 << shift);
        adc->SMPR2 |= (config->sample_time << shift);
    } else {
        // Channels 10-17 use SMPR1
        uint32_t shift = (config->channel - 10) * 3;
        adc->SMPR1 &= ~(0x07 << shift);
        adc->SMPR1 |= (config->sample_time << shift);
    }

    // 6. Configure regular sequence: 1 conversion, 1 channel
    adc->SQR1 = (0 << 20);          // 1 conversion (L[3:0] = 0 = 1 conversion)
    adc->SQR3 = config->channel;    // Only channel in sequence

    // 7. Set continuous mode if requested
    if (config->continuous) {
        adc->CR2 |= ADC_CR2_CONT;
    }

    // 8. Start conversion LAST
    adc->CR2 |= ADC_CR2_SWSTART;
}

uint16_t adc_read(volatile ADC_reg_t *adc){
    // For single conversion: start, wait, read
    if (!(adc->CR2 & ADC_CR2_CONT)) {
        adc->CR2 |= ADC_CR2_SWSTART;        // Start conversion
        while(!(adc->SR & ADC_SR_EOC));     // Wait for completion
    }

    // Read result (continuous mode always has latest value)
    return (uint16_t)(adc->DR & 0xFFF);     // 12-bit result
}

void adc_start(volatile ADC_reg_t *adc){
    adc->CR2 |=  ADC_CR2_SWSTART;
}

void adc_stop(volatile ADC_reg_t *adc){
    adc->CR2 &= ~ADC_CR2_ADON;
}

// Initialize ADC1: Channel 0, continuous mode
const ADC_config_t ADC_CH0_CNT_DEFAULT = {
    .channel = ADC_CH0,
    .sample_time = ADC_SMP_55_5_CYCLES,
    .prescaler = RCC_CFGR_ADCPRE_DIV4,
    .continuous = true,
};