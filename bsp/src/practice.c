#include "timers.h"

static tim_pwm_capture_t pwm_capture_data = {0};

void timer_ic_init(volatile TIM_reg_t *tim, const tim_ic_config_t *config, uint32_t timer_clock_hz) {
    // 1. setup variables and store the clock frequency
    pwm_capture_data.timer_clock_hz = timer_clock_hz;
    volatile uint32_t *ccmr;
    uint32_t ccmr_shift, cce_bit, ccp_bit;

    // 2. Determine CCMR register and bit positions based on channel
    ccmr = (config->channel <= TIM_CH2) ? &tim->CCMR1 : &tim->CCMR2;
    ccmr_shift = (config->channel == TIM_CH2 || config->channel == TIM_CH4) ? 8 : 0;

    // 3. Calculate CCER bit positions
    cce_bit = BIT(config->channel * 4);
    ccp_bit = BIT((config->channel * 4) + 1);

    // 4. Disable the channel and counter during configuration
    tim->CCER &= ~(cce_bit);
    tim->CR1 &= ~TIM_CR1_CEN;

    tim->CR1 &= ~(TIM_CR1_CKD_MASK | TIM_CR1_CMS_MASK | TIM_CR1_DIR);
    tim->CR1 |= config->clock_div;

    // 5. Clear the entire 8-bit slot for this channel in the CCMR register
    *ccmr &= ~(0xFFU << ccmr_shift);

    // ===== Standard Input Capture (Direct or Indirect) =====
    // 6. Map the input routing (CCxS bits)
    uint32_t ccxs_val = 0;
    switch (config->ic_mode) {
        case TIM_IC_MODE_DIRECT:
            ccxs_val = TIM_CCMRx_CCxS_INPUT_TI1;
        case TIM_IC_MODE_INDIRECT:
            ccxs_val = TIM_CCMRx_CCxS_INPUT_TI2;
        default:
            ccxs_val = TIM_CCMRx_CCxS_INPUT_TI1;
            break;
    }

    // 7. Combine and apply CCxS, prescaler, and filter
    uint32_t ccmr_val = (ccxs_val | config->prescaler | config->filter) << ccmr_shift;
    *ccmr = ccmr_val;

    // 8. Configure edge detection in CCER 
    if (config->edge == TIM_IC_EDGE_FALLING) {
        tim->CCER |= ccp_bit;
    } else {
        tim->CCER &= ~ccp_bit;
    }

    // 9. Enable the channel
    tim->CCER |= cce_bit;

    // 10. Configure interrupt in DIER
    if (config->enable_interrupt) {
        switch (config->channel) {
            case TIM_CH1: tim->DIER |= TIM_DIER_CC1IE; break;
            case TIM_CH2: tim->DIER |= TIM_DIER_CC2IE; break;
            case TIM_CH3: tim->DIER |= TIM_DIER_CC3IE; break;
            case TIM_CH4: tim->DIER |= TIM_DIER_CC4IE; break;
        }
    }

    // 11. Set prescaler to 0 (timer counts at full speed for capture)
    tim->PSC = 0;

    // 12. Set ARR to max (don't limit the counter)
    tim->ARR = 0xFFFF;

    // 13. Re-enable the counter
    tim->CR1 |= TIM_CR1_CEN;
}