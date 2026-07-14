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

    // 3.5. Enable internal channels BEFORE calibration
    if (config->channel >= ADC_CH16) {
        adc->CR2 |= ADC_CR2_TSVREFE;
        // CRITICAL: Long delay for internal channel wake-up!
        for (volatile uint32_t i = 0; i < 100000; i++);
    }   

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
    
    // second ADON writes triggers the actual start of conversion loop in continuous mode?
    adc->CR2 |= ADC_CR2_ADON;

    // 8. Start conversion LAST
    //adc->CR2 |= ADC_CR2_SWSTART;
}

uint16_t adc_read(volatile ADC_reg_t *adc){
    // For single conversion: start, wait, read
    if (!(adc->CR2 & ADC_CR2_CONT)) {
        adc->CR2 |= ADC_CR2_SWSTART;        // Start conversion
        while(!(adc->SR & ADC_SR_EOC));     // Wait for completion
    }

    // Read result (continuous mode always has latest value)
    return (uint16_t)(adc->DR);     // 12-bit result
}

void adc_start(volatile ADC_reg_t *adc){
    adc->CR2 |=  ADC_CR2_SWSTART;
}

void adc_stop(volatile ADC_reg_t *adc){
    adc->CR2 &= ~ADC_CR2_ADON;
}

int32_t convert_internal_temp(uint16_t adc_raw){
    // Vdda = 3300 mV, 12-bit ADC = 4095
    // V_sense (mV) = (adc_raw * 3300) / 4095
    int32_t v_sense_mv = ((int32_t)adc_raw * 3300) / 4095;

    // V25 = 1430 mV
    // Avg_Slope = 4.3 mV/C (multiplied by 100 to preserve precision = 430)
    // Formula scaled: Temp = ((V25 - V_sense) * 100) / 4.3 + 2500
    // Simplified:     Temp = ((1430 - v_sense_mv) * 10000) / 430 + 2500
    int32_t temp_hundredths = (((1430 - v_sense_mv) * 10000) / 430) + 2500;

    return temp_hundredths;
}

// Initialize ADC1: Channel 0, continuous mode
const ADC_config_t ADC_CH0_TEST = {
    .channel = ADC_CH0,
    .sample_time = ADC_SMP_55_5_CYCLES,
    .prescaler = RCC_CFGR_ADCPRE_DIV4,
    .continuous = true,
};

// Initialize ADC1: Internal voltage, continuous mode
const ADC_config_t ADC_CH17_VREFINT = {
    .channel = ADC_CH16,                    // Internal voltage reference
    .sample_time = ADC_SMP_239_5_CYCLES,
    .prescaler = RCC_CFGR_ADCPRE_DIV4,
    .continuous = true,
};

// Initialize ADC1: Channel 1, continuous mode
const ADC_config_t ADC_CH1_TEST = {
    .channel = ADC_CH1,             // PA1 instead of PA0
    .sample_time = ADC_SMP_55_5_CYCLES,
    .prescaler = RCC_CFGR_ADCPRE_DIV4,
    .continuous = true,
};