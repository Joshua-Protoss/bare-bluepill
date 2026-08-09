#include "timers.h"

static uint32_t tim_ic_clocks[14] = {0};            // One per timer instance
static tim_pwm_capture_t pwm_capture_data = {0};

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
void tim_ic_init(volatile TIM_reg_t *tim, const tim_ic_config_t *config, uint32_t tim_clock_hz) {
    // Determine timer index and store clock
    uint32_t tim_index = ((uint32_t)tim - TIM2_BASE) / 0x400;
    tim_ic_clocks[tim_index % 14] = tim_clock_hz;

    volatile uint32_t *ccmr;
    uint32_t ccmr_shift, cce_bit, ccp_bit;

    // Determine CCMR register and bit positions based on channel
    ccmr = (config->channel <= TIM_CH2) ? &tim->CCMR1 : &tim->CCMR2;
    ccmr_shift = (config->channel == TIM_CH2 || config->channel == TIM_CH4) ? 8 : 0;

    // Calculate CCER bit positions
    cce_bit = BIT(config->channel * 4);                                     // CC1E=bit0, CC2E=bit4, CC3E=bit8, CC4E=bit12
    ccp_bit = BIT((config->channel * 4) + 1);                               // CC1P=bit1, CC2P=bit5, CC3P=bit9, CC4P=bit13

    // Disable the channel and counter during configuration (RM0008 rule!)
    tim->CCER &= ~(cce_bit);
    tim->CR1 &= ~TIM_CR1_CEN;

    // Clear the entire 8-bit slot for this channel in the CCMR register
    *ccmr &= ~(0xFFU << ccmr_shift);                                        // This safely clears CCxS, ICxPSC, and ICxF all at once.   
    
    // Handle PWM input mode specially
    if (config->ic_mode == TIM_IC_MODE_PWM_INPUT) {                         // PWM input only works on CH1 or CH3 (the first channel of the pair)
        if (config->channel != TIM_CH1 && config->channel != TIM_CH3) {     // Validate channel selection
            return;
        }
        if (config->channel == TIM_CH1) {
            // ===== CH1 Configuration (Primary - measures period) =====
            uint32_t ch1_ccmr = TIM_CCMRx_CCxS_INPUT_TI1;                   // CC1S = 01: CH1 mapped to TI1 (direct input)
            ch1_ccmr |= config->prescaler | config->filter;
            tim->CCMR1 &= ~0xFFU;                                           // Clear CH1 slot
            tim->CCMR1 |= ch1_ccmr;

            // ===== CH2 Configuration (Secondary - measures duty) =====
            uint32_t ch2_ccmr = TIM_CCMRx_CCxS_INPUT_TI2;                   // CC2S = 10: CH2 mapped to TI1 (cross-mapped)
            ch2_ccmr |= config->prescaler | config->filter;
            tim->CCMR1 &= ~(0xFFU << 8);                                    // Clear CH2 slot
            tim->CCMR1 |= (ch2_ccmr << 8);

            // ===== Edge Polarity Configuration =====
            tim->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC2P);                  // CH1 captures RISING edge (CC1P = 0)
            tim->CCER |= TIM_CCER_CC2P;                                     // CH2 captures FALLING edge (CC2P = 1)
            tim->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E;                     // Enable BOTH channels

            // Enable interrupts if requested
            if (config->enable_interrupt) {
                tim->DIER |= TIM_DIER_CC1IE;                                // Only need CH1 interrupt
            }
            
        } else if (config->channel == TIM_CH3) {
            // ===== CH3 Configuration (Primary - measures period) =====
            uint32_t ch3_ccmr = TIM_CCMRx_CCxS_INPUT_TI1;                   // CH3 mapped to TI3 (direct)
            ch3_ccmr |= config->prescaler | config->filter;
            tim->CCMR2 &= ~0xFFU;                                           // Clear CH3 slot
            tim->CCMR2 |= ch3_ccmr;

            // ===== CH4 Configuration (Secondary - measures duty) =====
            uint32_t ch4_ccmr = TIM_CCMRx_CCxS_INPUT_TI2;                   // CC2S = 10: CH4 mapped to TI1 (cross-mapped)
            ch4_ccmr |= config->prescaler | config->filter;
            tim->CCMR2 &= ~(0xFFU << 8);                                    // Clear CH4 slot
            tim->CCMR2 |= (ch4_ccmr << 8);

            // ===== Edge Polarity Configuration =====
            tim->CCER &= ~(TIM_CCER_CC3P | TIM_CCER_CC4P);                  // CH3 captures RISING edge (CC3P = 0)
            tim->CCER |= TIM_CCER_CC4P;                                     // CH4 captures FALLING edge (CC2P = 1)
            tim->CCER |= TIM_CCER_CC3E | TIM_CCER_CC4E;                     // Enable BOTH channels

            if (config->enable_interrupt) {
                tim->DIER |= TIM_DIER_CC3IE;                                // Only need CH3 interrupt
            }
        }
    } else {
        // ===== Standard Input Capture (Direct or Indirect) =====
        // Map the input routing (CCxS bits)
        uint32_t ccxs_val = 0;
        switch (config->ic_mode) {
            case TIM_IC_MODE_DIRECT:                                        // Channel 1->TI1, Ch 2->TI2, Ch 3->TI3, Ch 4->TI4
                ccxs_val = TIM_CCMRx_CCxS_INPUT_TI1;
                break;
            case TIM_IC_MODE_INDIRECT:                                      // Cross-mapped (Ch 1->TI2, Ch 2->TI1, etc.)
                ccxs_val = TIM_CCMRx_CCxS_INPUT_TI2;
                break;
            default:
                ccxs_val = TIM_CCMRx_CCxS_INPUT_TI1;
                break;
        }

        // Combine and apply CCxS, Prescaler, and Filter
        uint32_t ccmr_val = (ccxs_val | config->prescaler | config->filter) << ccmr_shift;
        *ccmr = ccmr_val;

        // Configure edge detection in CCER
        if (config->edge == TIM_IC_EDGE_FALLING) {
            tim->CCER |= ccp_bit;                                           // 1 = Falling edge active
        } else {
            tim->CCER &= ~ccp_bit;                                          // 0 = Rising edge active (Default / fallback for BOTH)
        }

        tim->CCER |= cce_bit;                                               // Enable the channel

        // Configure Interrupts in DIER
        if (config->enable_interrupt) {
            switch (config->channel) {
                case TIM_CH1: tim->DIER |= TIM_DIER_CC1IE; break;
                case TIM_CH2: tim->DIER |= TIM_DIER_CC2IE; break;
                case TIM_CH3: tim->DIER |= TIM_DIER_CC3IE; break;
                case TIM_CH4: tim->DIER |= TIM_DIER_CC4IE; break;
            }
        }
    }
    
    // Set prescaler to 0 (timer counts at full speed for capture)
    tim->PSC = 0;

    // Set ARR to max (don't limit the counter)
    tim->ARR = 0xFFFF;

    // Re-enable the counter
    tim->CR1 |= TIM_CR1_CEN;
}

uint32_t tim_ic_get_capture(volatile TIM_reg_t *tim, tim_channel_t channel) {
    volatile uint32_t *ccr = tim_get_ccr(tim, channel);
    return *ccr;
}

float tim_ic_calculate_frequency(volatile TIM_reg_t *tim, tim_channel_t channel) {
    static uint32_t last_capture[4] = {0};
    uint32_t current = tim_ic_get_capture(tim, channel);
    uint32_t period;

    if (current >= last_capture[channel]) {
        period = current - last_capture[channel];
    } else {
        period = (0xFFFF - last_capture[channel]) + current + 1;
    }

    last_capture[channel] = current;
    if (period == 0) return 0.0f;

    uint32_t tim_index = ((uint32_t)tim - TIM2_BASE) / 0x400; 
    float timer_clock = (float) tim_ic_clocks[tim_index % 14];
    if (timer_clock == 0.0f) timer_clock = 44000000.0f;                 // system clock = 44 MHz
    return timer_clock / (float)period;                             // Need timer clock info
}

float tim_ic_calculate_period_ms(volatile TIM_reg_t *tim, tim_channel_t channel) {
    float freq = tim_ic_calculate_frequency(tim, channel);
    if (freq == 0.0f) return 0.0f;
    return 1000.0f / freq;
}

// PWM input mode (for reading RC signals, etc.)
float tim_ic_get_duty_cycle(volatile TIM_reg_t *tim) {
    uint32_t period = tim->CCR1;
    uint32_t duty = tim->CCR2;
    if (period == 0) return 0.0f;
    return ((float)duty / (float)period) * 100.0f;
}

uint32_t tim_ic_get_period(volatile TIM_reg_t *tim) {
    return tim->CCR1;
}

tim_pwm_capture_t* tim_ic_get_pwm_capture(void) {
    return &pwm_capture_data;
}

bool tim_ic_is_new_data_ready(void) {
    return pwm_capture_data.new_data;
}

void tim_ic_clear_new_data(void) {
    pwm_capture_data.new_data = false;
}

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

// Input capture PWM test
// Generate 1kHz PWM with 30% duty cycle
const tim_oc_config_t PWM_CH1_1KHZ_30 = {
    .frequency = 1000,
    .duty_cycle = 30,
    .channel = TIM_CH1,
    .oc_mode = TIM_OC_MODE_PWM1,
    .op_mode = TIM_MODE_CONTINUOUS,
    .clock_div = TIM_CKD_DIV1,
    .cms_mode = TIM_CMS_EDGE,
    .direction = TIM_DIR_UP,
};

// Configure TIM3 CH1 to read PWM in PWM input mode
const tim_ic_config_t INPUT_CAPTURE_RISING_44MHZ = {
    .channel = TIM_CH1,
    .ic_mode = TIM_IC_MODE_DIRECT,
    .edge = TIM_IC_EDGE_RISING,
    .prescaler = TIM_IC_PSC_DIV1,
    .filter = TIM_IC_FILTER_NONE,
    .enable_interrupt = true,
};