#include "tim.h"

// Helper: Get CCR register for channel
static volatile uint32_t *tim_get_ccr(TIM_reg_t *tim, tim_channel_t channel) {
    switch (channel) {
        case TIM_CH1: return &tim->CCR1;
        case TIM_CH2: return &tim->CCR2;
        case TIM_CH3: return &tim->CCR3;
        case TIM_CH4: return &tim->CCR4;
        default:      return &tim->CCR1;
    }
}

// Helper: Calculate prescaler and ARR for desired frequency
static bool tim_calculate_prescaler_arr(uint32_t tim_clock, uint32_t desired_freq, uint32_t *psc, uint32_t *arr){
    for (uint32_t psc_try = 0; psc_try <= 0xFFFF; psc_try++) {
        uint32_t arr_try = (tim_clock / ((psc_try + 1) * desired_freq)) - 1;

        if (arr_try <= 0xFFFF) {
            *psc = psc_try;
            *arr = arr_try;
            return true;
        }
    }
    return false; // Cannot achieve this frequency
}

void tim_pwm_init(TIM_reg_t *tim, const tim_pwm_config_t *config, uint32_t tim_clock) {
    uint32_t psc, arr;

    // pre-calculate prescaler and auto-reload
    if (!tim_calculate_prescaler_arr(tim_clock, config->frequency, &psc, &arr)) {
        return; // invalid frequency
    }

    // Disable counter before configuration
    tim->CR1 &= ~TIM_CR1_CEN;

    // Set prescaler and auto-reload
    tim->PSC = psc;
    tim->ARR = arr;

    // Configure PWM mode 1 based on channel
    uint32_t ccmr_offset = (config->channel <= TIM_CH2) ? 0 : 1;
    volatile uint32_t *ccmr = (ccmr_offset == 0) ? &tim->CCMR1 : &tim->CCMR2;

    // Shift based on channel (CH1/CH3 = bits 0-7, CH2/CH4 = bits 8-15)
    uint8_t shift = ((config->channel & 1) == 0) ? 4 : 12;

    // Set PWM mode 1
    *ccmr &= ~(0x07 << shift);                          // Clear OC mode bits
    *ccmr |= (TIM_CCMR1_OC1M_PWM1 << (shift - 4));        // Set PWM1
    *ccmr |= (TIM_CCMR1_OC1PE << (shift - 4 - 1 ));         // Preload enable

    // Set initial duty cycle
    tim_pwm_set_duty(tim, config->channel, config->duty_cycle);

    // Enable output for this channel
    uint32_t ccer_bit = (config->channel == TIM_CH1) ? TIM_CCER_CC1E :
                        (config->channel == TIM_CH2) ? TIM_CCER_CC2E :
                        (config->channel == TIM_CH3) ? TIM_CCER_CC3E :
                        TIM_CCER_CC4E;

    tim->CCER |= ccer_bit;

    // Enable auto-reload
    tim->CR1 |= TIM_CR1_ARPE;

    // Generate update to load prescaler and ARR
    tim->EGR |= TIM_EGR_UG;

    // Enable counter
    tim->CR1 |= TIM_CR1_CEN;
}

void tim_pwm_set_duty(TIM_reg_t *tim, tim_channel_t channel, uint8_t duty_cycle) {
    if (duty_cycle > 100) {
        duty_cycle = 100;
    }

    uint32_t arr = tim->ARR;
    uint32_t ccr_value = (arr + 1) * duty_cycle / 100;

    volatile uint32_t *ccr = tim_get_ccr(tim, channel);
    *ccr = ccr_value;
}

void tim_enable(TIM_reg_t *tim){
    tim->CR1 |= TIM_CR1_CEN;
}

void tim_disable(TIM_reg_t *tim){
    tim->CR1 &= ~TIM_CR1_CEN;
}