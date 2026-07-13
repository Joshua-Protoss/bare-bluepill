#include "systick.h"
#include "common.h"

// Set reload value in systick reload value register
void systick_set_reload(uint32_t reload){

    SYSTICK->SYST_RVR = (SYSTICK_RELOAD_MAX & reload); // The ARM Cortex-M processors ignores bits 24-31 automatically, but I put this for safety anyway
    SYSTICK->SYST_CVR = 0;                     // Any write to CVR clears it and forces RVR to reload
}

// Get reload value in systick reload value register
uint32_t systick_get_reload(void){
   return SYSTICK->SYST_RVR & SYSTICK_RELOAD_MAX;
}

// Initialize Systick
bool systick_set_frequency(uint32_t desired_freq, uint32_t ahb_freq){
    
    if (desired_freq == 0 || ahb_freq == 0){
        return false;
    }
    
    // Stop the timer and clear the current value register safely
    SYSTICK->SYST_CSR &= ~SYST_CSR_ENABLE;
    SYSTICK->SYST_CVR = 0;
    
    uint32_t ticks = ahb_freq/desired_freq;

    // is the required tick fits in the 24-bit register?
    if ((ticks - 1) <= SYSTICK_RELOAD_MAX){
        SYSTICK->SYST_CSR |= SYST_CSR_CLKSOURCE;        // set it to 1, AHB freq
    } else {
        ticks = (ahb_freq / 8) / desired_freq;

        // checks again
        if ((ticks - 1) <= SYSTICK_RELOAD_MAX){
            SYSTICK->SYST_CSR &= ~(SYST_CSR_CLKSOURCE); // clear CLKSOURCE bit, set it to 0 (AHB/8)
        } else {
            // Target frequency is too slow! Even with /8, the number overflows 24 bits.
            return false;
        }
    }

    systick_set_reload(ticks - 1);

    // Enable timer and interrupt by default
    SYSTICK->SYST_CSR |= (SYST_CSR_ENABLE | SYST_CSR_TICKINT);

    return true;
}

void systick_timer_enable(void){
    SYSTICK->SYST_CSR |= SYST_CSR_ENABLE;
}

void systick_timer_disable(void){
    SYSTICK->SYST_CSR &= ~SYST_CSR_ENABLE;
}

void systick_interrupt_enable(void){
    SYSTICK->SYST_CSR |= SYST_CSR_TICKINT;
}

void systick_interrupt_disable(void){
    SYSTICK->SYST_CSR &= ~SYST_CSR_TICKINT;
}

bool systick_get_countflag(void){
   return (SYSTICK->SYST_CSR & SYST_CSR_COUNTFLAG) != 0;
}

uint32_t systick_get_value(void){
    return SYSTICK->SYST_CVR & SYSTICK_CURRENT_VALUE_MASK;
}

void systick_clear_counter(void){
    SYSTICK->SYST_CVR = 0;
}

uint32_t systick_get_calib(void){
    return (SYSTICK->SYST_CALIB & SYSTICK_CALIB_TENMS);
}

void systick_delay_ms(uint32_t ms){
    for (uint32_t i = 0; i < ms; i++) {
        while(!systick_get_countflag());
    }
}