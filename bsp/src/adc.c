#include "adc.h"

void adc_init(volatile ADC_reg_t *adc, const ADC_config_t *config){
    
    // 1. Set ADC clock prescaler (must be ≤ 14 MHz!)
    RCC_CFGR &= ~(0x03 << RCC_CFGR_ADCPRE_SHIFT);          // Clear ADCPRE bits
    RCC_CFGR |= (config->prescaler << RCC_CFGR_ADCPRE_SHIFT);

    // 2. Enable ADC clock
    rcc_periph_clock_enable(adc == ADC1 ? RCC_ADC1 : RCC_ADC2);

    // 3. Power on ADC (Wake up the peripheral from power-down mode) + small delay at least 3-4 microseconds
    adc->CR2 |= ADC_CR2_ADON; 
    for (volatile uint32_t i = 0; i < 10000; i++);

    // 4. Enable internal channels BEFORE calibration
    if (config->channel >= ADC_CH16) {
        adc->CR2 |= ADC_CR2_TSVREFE;
        // CRITICAL: Long delay for internal channel wake-up!
        for (volatile uint32_t i = 0; i < 100000; i++);
    }   

    // 5. Calibrate ADC
    adc->CR2 |= ADC_CR2_RSTCAL;                                         // Reset calibration
    while(adc->CR2 & ADC_CR2_RSTCAL);                                   // Wait for reset calibration complete (hardware clears)
    adc->CR2 |= ADC_CR2_CAL;
    while(adc->CR2 & ADC_CR2_CAL);                                      // Wait for calibration complete

    // 6. Set sample time for the channel
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

    // 7. Configure regular sequence: 1 conversion, 1 channel
    adc->SQR1 = (0 << ADC_SQR1_CONV_NUM_SHIFT);          // 1 conversion (L[3:0] = 0 = 1 conversion)
    adc->SQR3 = config->channel;                         // Clear SQ1-SQ6 and only set 1 channel in sequence

    // 8. Set continuous mode if requested
    if (config->continuous) {
        adc->CR2 |= ADC_CR2_CONT;
    }
    
    // second ADON writes triggers the actual start of conversion loop in continuous mode
    adc->CR2 |= ADC_CR2_ADON;
}

void adc_scan_init(volatile ADC_reg_t *adc, const ADC_scan_config_t *config){

    // 1. Set ADC clock prescaler (must be ≤ 14 MHz!)
    RCC_CFGR &= ~(0x03 << RCC_CFGR_ADCPRE_SHIFT);           // Clear ADCPRE bits
    RCC_CFGR |= (config->prescaler << RCC_CFGR_ADCPRE_SHIFT);

    // 2. Enable ADC clock
    rcc_periph_clock_enable(adc == ADC1 ? RCC_ADC1 : RCC_ADC2);

    // 3. Power on ADC (Wake up the peripheral from power-down mode) + small delay at least 3-4 microseconds
    adc->CR2 |= ADC_CR2_ADON;
    for (volatile uint32_t i = 0; i < 10000; i++);

    // 4. Enable internal channels BEFORE calibration if any channel >= 16
    for (uint8_t i = 0; i < config->num_channels; i++) {
        if(config->channels[i] >= ADC_CH16) {
            adc->CR2 |= ADC_CR2_TSVREFE;
            // CRITICAL: Long delay for internal channel wake-up!
            for (volatile uint32_t i = 0; i < 100000; i++);
            break;
        }
    }

    // 5. Calibrate ADC
    adc->CR2 |= ADC_CR2_RSTCAL;                                 // Reset calibration 
    while(adc->CR2 & ADC_CR2_RSTCAL);                        // Wait for reset calibration complete (hardware clears)
    adc->CR2 |= ADC_CR2_CAL;
    while(adc->CR2 & ADC_CR2_CAL);                           // Wait for calibration complete

    // 6. Set sample time for ALL channels
    for (uint8_t i = 0; i < config->num_channels; i++) {
        if (config->channels[i] <= ADC_CH9) {
            // channels[i]s 0-9 use SMPR2
            uint32_t shift = (config->channels[i]) * 3;
            adc->SMPR2 &= ~(0x07 << shift);
            adc->SMPR2 |= (config->sample_time << shift);
        } else {
            // channels[i]s 10-17 use SMPR1
            uint32_t shift = (config->channels[i] - 10) * 3;
            adc->SMPR1 &= ~(0x07 << shift);
            adc->SMPR1 |= (config->sample_time << shift);
        }
    }

    // 7. Configure scan sequence, SQR1: bits 23-20 = number of conversions (1-16)
    uint8_t n = config->num_channels;
    adc->SQR1 = ((n - 1) << ADC_SQR1_CONV_NUM_SHIFT);

    // Clear all SQ bits first
    adc->SQR1 &= ~0x000FFFFF;                                   // Clear SQ13-SQ16
    adc->SQR2 = 0;                                              // Clear SQ7-SQ12
    adc->SQR3 = 0;                                              // Clear SQ1-SQ6

    // Fill in sequence
    for (uint8_t i = 0; i < n; i++) {
        uint8_t sq_num = i + 1;                                             // increment SQ, SQ1, SQ2, SQ3

        if (sq_num <= 6) {                                                  // SQ1-SQ6 go in SQR3
            adc->SQR3 |= (config->channels[i] << ((sq_num - 1) * 5));
        } else if (sq_num <= 12) {                                          // SQ7-SQ12 go in SQR2
            adc->SQR2 |= (config->channels[i] << ((sq_num - 7) * 5));
        } else {                                                            // SQ13-SQ16 go in SQR1
            adc->SQR1 |= (config->channels[i] << ((sq_num - 13) * 5));
        }
    }

    // 8. Enable scan mode in CR1
    adc->CR1 |= ADC_CR1_SCAN;

    if (config->continuous) {                       // Set continuous mode if requested
        adc->CR2 |= ADC_CR2_CONT;
        adc->CR2 |= ADC_CR2_ADON;                       // Second ADON to start conversion
    }

}

void adc_scan_read(volatile ADC_reg_t *adc, uint16_t *buffer, uint8_t count) {
    if(!(adc->CR2 & ADC_CR2_CONT)) {
        // Wait for any ongoing conversion to finish
        while(adc->SR & ADC_SR_STRT);                               // Wait for STRT=0 (not busy)
        adc->CR2 |= ADC_CR2_ADON;
        adc->CR2 |= ADC_CR2_SWSTART;                                // Single scan: start and read each result
        // Debug: Check if SWSTART was accepted
        volatile uint32_t cr2_check = adc->CR2;
        // Should show SWSTART bit set
        for (uint8_t i = 0; i < count; i++) {
            while(!(adc->SR & ADC_SR_EOC));                         // Wait for completion
            buffer[i] = (uint16_t) (adc->DR & 0xFFF);
        }
    } else {
        // Continuous scan: DR only has the LAST channel!
        // You need DMA for multi-channel continuous mode!
        buffer[0] = (uint16_t) (adc->DR & 0xFFF);                   // Only last channel available!
    }
}

uint16_t adc_read(volatile ADC_reg_t *adc){
    // For single conversion: start, wait, read
    if (!(adc->CR2 & ADC_CR2_CONT)) {
        adc->CR2 |= ADC_CR2_SWSTART;                                // Start conversion
        while(!(adc->SR & ADC_SR_EOC));                             // Wait for completion
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

    // Display as Celsius with 2 decimal places:
    //int32_t temp_whole = temp / 100;      // 34
    //int32_t temp_frac = temp % 100;       // 76

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
const ADC_config_t ADC_CH16_VREFINT = {
    .channel = ADC_CH16,                    // Internal temperature
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

// Initialize ADC1: scan mode
const ADC_scan_config_t ADC_SCAN_TEST = {
    .channels = {ADC_CH1, ADC_CH16},            // CH1 : potentiometer, CH16 : internal temperature sensor
    .num_channels = 2,
    .sample_time = ADC_SMP_239_5_CYCLES,
    .prescaler = RCC_CFGR_ADCPRE_DIV4,
    .continuous = false,
};