#include "timers.h"

// Helper: Get CCR register for channel
static volatile uint32_t *tim_get_ccr(volatile TIM_reg_t *tim, tim_channel_t channel) {
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

void tim_pwm_init(volatile TIM_reg_t *tim, const tim_pwm_config_t *config, uint32_t tim_clock) {
    uint32_t psc, arr;

    // pre-calculate prescaler and auto-reload
    if (!tim_calculate_prescaler_arr(tim_clock, config->frequency, &psc, &arr)) {
        return; // invalid frequency
    }

    // Disable counter before configuration
    tim->CR1 &= ~TIM_CR1_CEN;

    // Apply CR1 settings
    tim->CR1 &= ~(TIM_CR1_CKD_MASK | TIM_CR1_CMS_MASK | TIM_CR1_DIR);
    tim->CR1 |= config->clock_div | config->cms_mode | config->direction;

    // if one pulse mode selected
    if (config->op_mode == TIM_MODE_PWM_ONE_SHOT) {
        tim->CR1 |= TIM_CR1_OPM;  // One Pulse Mode
    }

    // Set prescaler and auto-reload
    tim->PSC = psc;
    tim->ARR = arr;

    // Configure selected output compare mode for the channel
    switch (config->channel) {
        case TIM_CH1:
            tim->CCMR1 &= ~TIM_CCMR1_OC1M_MASK;
            tim->CCMR1 |= config->oc_mode | TIM_CCMR1_OC1PE;
            tim->CCER |= TIM_CCER_CC1E;
            break;
        case TIM_CH2:
            tim->CCMR1 &= ~(TIM_CCMR1_OC1M_MASK << 8);  // OC2 bits are +8 from OC1
            tim->CCMR1 |= (config->oc_mode << 8) | (TIM_CCMR1_OC1PE << 8);
            tim->CCER |= TIM_CCER_CC2E;
            break;
        case TIM_CH3:
            tim->CCMR2 &= ~TIM_CCMR1_OC1M_MASK;  // Same mask works, CCMR2 uses same bit positions
            tim->CCMR2 |= config->oc_mode | TIM_CCMR1_OC1PE;
            tim->CCER |= TIM_CCER_CC3E;
            break;
        case TIM_CH4:
            tim->CCMR2 &= ~(TIM_CCMR1_OC1M_MASK << 8);
            tim->CCMR2 |= (config->oc_mode << 8) | (TIM_CCMR1_OC1PE << 8);
            tim->CCER |= TIM_CCER_CC4E;
            break;
        default:
            return;
    }

    // Set initial duty cycle
    tim_pwm_set_duty(tim, config->channel, config->duty_cycle);

    // Enable auto-reload preload
    tim->CR1 |= TIM_CR1_ARPE;

    // Generate update to load prescaler and ARR
    tim->EGR |= TIM_EGR_UG;

    // Enable counter
    tim->CR1 |= TIM_CR1_CEN;
}

void tim_pwm_set_duty(volatile TIM_reg_t *tim, tim_channel_t channel, uint8_t duty_cycle) {
    if (duty_cycle > 100) {
        duty_cycle = 100;
    }

    uint32_t arr = tim->ARR;
    uint32_t ccr_value = (arr + 1) * duty_cycle / 100;

    volatile uint32_t *ccr = tim_get_ccr(tim, channel);
    *ccr = ccr_value;
}

void tim_enable(volatile TIM_reg_t *tim){
    tim->CR1 |= TIM_CR1_CEN;
}

void tim_disable(volatile TIM_reg_t *tim){
    tim->CR1 &= ~TIM_CR1_CEN;
}

// PWM configuration: 1kHz, 50% duty cycle on CH1
const tim_pwm_config_t PWM_CH1_1KHZ_50 = {
        .frequency = 1000,
        .duty_cycle = 50,
        .channel = TIM_CH1,
        .oc_mode = TIM_OC_MODE_PWM1,
        .op_mode = TIM_MODE_PWM_CONTINUOUS,
        .clock_div = TIM_CKD_DIV1,    // Default
        .cms_mode = TIM_CMS_EDGE,      // Default
        .direction = TIM_DIR_UP,       // Default
};

// CH2: LED2 fading out (opposite phase)
const tim_pwm_config_t PWM_CH2_1KHZ_50 = {
        .frequency = 1000,
        .duty_cycle = 50,
        .channel = TIM_CH2,          // ← Channel 2!
        .oc_mode = TIM_OC_MODE_PWM1,
        .op_mode = TIM_MODE_PWM_CONTINUOUS,
        .clock_div = TIM_CKD_DIV1,    // Default
        .cms_mode = TIM_CMS_EDGE,      // Default
        .direction = TIM_DIR_UP,       // Default
};