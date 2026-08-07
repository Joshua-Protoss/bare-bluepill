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
    // Check if the frequency is physically too slow to store in ARR * PSC
    if (desired_freq == 0 || (tim_clock / desired_freq) > (0x10000ULL * 0x10000ULL)) {
        return false;
    }

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

void tim_oc_init(volatile TIM_reg_t *tim, const tim_oc_config_t *config, uint32_t tim_clock) {
    uint32_t psc, arr, calculation_clock;
    calculation_clock = tim_clock;

    if (config->cms_mode != TIM_CMS_EDGE) {
        calculation_clock /= 2;
    }

    // pre-calculate prescaler and auto-reload
    if (!tim_calculate_prescaler_arr(calculation_clock, config->frequency, &psc, &arr)) {
        return;                                                         // invalid frequency
    }

    // Disable counter before configuration
    tim->CR1 &= ~TIM_CR1_CEN;

    // Apply CR1 settings
    tim->CR1 &= ~(TIM_CR1_CKD_MASK | TIM_CR1_CMS_MASK | TIM_CR1_DIR);
    tim->CR1 |= config->clock_div | config->cms_mode | config->direction;

    // if one pulse mode selected
    if (config->op_mode == TIM_MODE_ONE_PULSE) {
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
            tim->CCMR1 &= ~(TIM_CCMR1_OC1M_MASK << 8);                  // OC2 bits are +8 from OC1
            tim->CCMR1 |= (config->oc_mode << 8) | (TIM_CCMR1_OC1PE << 8);
            tim->CCER |= TIM_CCER_CC2E;
            break;
        case TIM_CH3:
            tim->CCMR2 &= ~TIM_CCMR1_OC1M_MASK;                         // Same mask works, CCMR2 uses same bit positions
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
    tim_oc_set_duty_cycle(tim, config->channel, config->duty_cycle);

    // Enable auto-reload preload
    tim->CR1 |= TIM_CR1_ARPE;

    // Generate update to load prescaler and ARR
    tim->EGR |= TIM_EGR_UG;

    // Configure TRGO if explicitly requested 
    if (config->trgo != TIM_TRGO_NONE) {
        tim->CR2 &= ~(0x7 << 4);
        tim->CR2 |= (config->trgo << 4);
    }

    // Enable output compare for advanced timer TIM1/TIM8
    if (tim == TIM1 || tim == TIM8){
        tim->BDTR |= TIM_BDTR_MOE;
    }

    // Enable counter
    tim->CR1 |= TIM_CR1_CEN;
}

void tim_oc_set_duty_cycle(volatile TIM_reg_t *tim, tim_channel_t channel, uint8_t duty_cycle) {
    if (duty_cycle > 100) {
        duty_cycle = 100;
    }

    uint32_t arr = tim->ARR;
    uint32_t ccr_value = ((uint64_t)(arr + 1) * duty_cycle) / 100;
    // uint64_t cast only needed to prevent overflow if the user make extreme cases, such as:
    // running 1Hz frequency with 99% duty cycle with 72 MHz clock frequency
    volatile uint32_t *ccr = tim_get_ccr(tim, channel);
    *ccr = ccr_value;
}

// Function for input mode
void tim_ic_init(volatile TIM_reg_t *tim, const tim_ic_config_t *config) {
    volatile uint32_t *ccmr;
    uint32_t ccmr_shift, ccer_bit_enable;

    // Determine CCMR register and bit positions based on channel
    ccmr = (config->channel <= TIM_CH2) ? &tim->CCMR1 : &tim->CCMR2;
    ccmr_shift = (config->channel == TIM_CH2 || config->channel == TIM_CH4) ? 8 : 0;

    // Calculate CCER bit positions

    // Disable counter during configuration
    tim->CR1 &= ~TIM_CR1_CEN;

    // Disable the channel before modifying configuration registers (RM0008 rule!)
}
uint32_t tim_ic_get_capture(volatile TIM_reg_t *tim, tim_channel_t channel);
float tim_ic_calculate_frequency(volatile TIM_reg_t *tim, tim_channel_t channel);
float tim_ic_calculate_period_ms(volatile TIM_reg_t *tim, tim_channel_t channel);

// PWM input mode (for reading RC signals, etc.)
void tim_ic_pwm_init(volatile TIM_reg_t *tim, tim_channel_t channel);
float tim_ic_get_duty_cycle(volatile TIM_reg_t *tim);
uint32_t tim_ic_get_period(volatile TIM_reg_t *tim);

void tim_enable(volatile TIM_reg_t *tim){
    tim->CR1 |= TIM_CR1_CEN;
}

void tim_disable(volatile TIM_reg_t *tim){
    tim->CR1 &= ~TIM_CR1_CEN;
}

// PWM configuration: 1kHz, 50% duty cycle on CH1
const tim_oc_config_t PWM_CH1_1KHZ_50 = {
        .frequency = 1000,
        .duty_cycle = 50,
        .channel = TIM_CH1,
        .oc_mode = TIM_OC_MODE_PWM1,
        .op_mode = TIM_MODE_CONTINUOUS,
        .clock_div = TIM_CKD_DIV1,    
        .cms_mode = TIM_CMS_EDGE,      
        .direction = TIM_DIR_UP,      
};

// CH2: LED2 fading out (opposite phase)
const tim_oc_config_t PWM_CH2_1KHZ_50 = {
        .frequency = 1000,
        .duty_cycle = 50,
        .channel = TIM_CH2,          // ← Channel 2!
        .oc_mode = TIM_OC_MODE_PWM1,
        .op_mode = TIM_MODE_CONTINUOUS,
        .clock_div = TIM_CKD_DIV1,    
        .cms_mode = TIM_CMS_EDGE,      
        .direction = TIM_DIR_UP,       
};

// ADC Trigger
const tim_oc_config_t TIM1_ADC_TRIG_1KHz = {
    .frequency = 1000,                                          // 1kHz ADC sampling rate
    .duty_cycle = 50,                                           // Trigger at 50% of period
    .channel = TIM_CH1,                                         // CC1 → ADC EXTSEL
    .oc_mode = TIM_OC_MODE_PWM1,
    .op_mode = TIM_MODE_CONTINUOUS,
    .clock_div = TIM_CKD_DIV1,
    .cms_mode = TIM_CMS_EDGE,
    .direction = TIM_DIR_UP,
    .trgo = TIM_TRGO_CC1,
};