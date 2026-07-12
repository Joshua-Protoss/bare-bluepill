#include "adc.h"
#include "rcc.h"

void adc_init(volatile ADC_reg_t *adc, const ADC_config_t *config){
    // 1. Enable ADC clock
    if (adc == ADC1){
        rcc_periph_clock_enable(RCC_ADC1);
    } else if (adc == ADC2) {
        rcc_periph_clock_enable(RCC_ADC2);
    }

    // 2. Power on ADC (Wake up from power-down mode)
    adc->CR2 |= ADC_CR2_ADON; 

    // Wait for ADC to stabilize (t_STAB = ~1μs)
    for (volatile uint32_t i = 0; i < 1000; i++);

    // 3. Calibrate ADC
    adc->CR2 |= ADC_CR2_RSTCAL;
    while(adc->CR2 & ADC_CR2_RSTCAL);       // Wait for reset calibration complete

    adc->CR2 |= ADC_CR2_CAL;
    while(adc->CR2 & ADC_CR2_CAL);          // Wait for calibration complete

    // 4. Set sample time for the channel
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

    // 5. Configure regular sequence: 1 conversion, 1 channel
    adc->SQR1 = (0 << 20);          // 1 conversion (L[3:0] = 0 = 1 conversion)
    adc->SQR3 = config->channel;    // Only channel in sequence

    // 6. Set continuous mode if requested
    if (config->continuous) {
        adc->CR2 |= ADC_CR2_CONT;
    }

    // 7. Start conversion if continuous
    if (config->continuous){
        adc->CR2 |= ADC_CR2_SWSTART;
    }
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
    adc->CR2 |= ADC_CR2_SWSTART;
}

void adc_stop(volatile ADC_reg_t *adc){
    adc->CR2 &= ~ADC_CR2_ADON;
}