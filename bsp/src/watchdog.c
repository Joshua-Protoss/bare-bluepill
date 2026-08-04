#include "watchdog.h"
#include "rcc.h"

void iwdg_init(IWDG_prescaler_t prescaler, uint16_t reload) {
    // CRITICAL: Enable LSI if not already running
    if (!(RCC->CSR & RCC_CSR_LSIRDY)) {
        RCC->CSR |= RCC_CSR_LSION;          // Turn on LSI
        while (!(RCC->CSR & RCC_CSR_LSIRDY)); // Wait for LSI to be ready
    }

    // Start the watchdog, This wakes up the synchronization logic so the SR flags actually work.
    IWDG->KR = IWDG_KR_KEY_ENABLE;

    // Unlock PR and RLR registers
    IWDG->KR = IWDG_KR_KEY_ACCESS;

    // Set prescaler
    IWDG->PR = prescaler;

    // Set reload value (12-bit: 0-4095)
    IWDG->RLR = reload & 0xFFFU;

    // Wait for registers to be updated
    while (IWDG->SR & BIT(0));                      // Wait for PVU
    while (IWDG->SR & BIT(1));                      // Wait for RVU

    // Refresh the counter immediately to start with a full countdown
    IWDG->KR = IWDG_KR_KEY_ENABLE;
}

void iwdg_kick(void) {
    IWDG->KR = IWDG_KR_KEY_RELOAD;
}

void iwdg_freeze_in_debug(void) {
    // DBGMCU_CR: Debug MCU Configuration Register
    // Bit 8: DBG_IWDG_STOP - Stop IWDG when core is halted
    DBGMCU_CR |= DBG_IWDG_STOP;
}

void wwdg_init(WWDG_prescaler_t prescaler, uint8_t reload, uint8_t window) {
     // Enable WWDG clock
     rcc_periph_clken(RCC_WWDG);

     // Set window value
     WWDG->CFR = (window & 0x7F) | prescaler;

     // Set counter and enable
     WWDG->CR = WWDG_CR_WDGA | (reload & 0x7F);
}

void wwdg_kick(uint8_t counter_value) {
    WWDG->CR = WWDG_CR_WDGA | (counter_value & 0x7F);
}

void wwdg_enable_ewi(void) {
    WWDG->CFR |= WWDG_CFR_EWI;
}

void wwdg_isr(void) {
    // Check if the Early Wakeup Interrupt flag is set
    if (WWDG->SR & WWDG_SR_EWIF) {
        // Clear the flag by writing 0 (per reference manual)
        WWDG->SR &= ~WWDG_SR_EWIF;
    }
}