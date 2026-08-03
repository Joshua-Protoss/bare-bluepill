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
    IWDG->RLR = IWDG_KR_KEY_RELOAD;
}

void iwdg_kick(void) {
    IWDG->KR = IWDG_KR_KEY_RELOAD;
}

void iwdg_freeze_in_debug(void) {
    // DBGMCU_CR: Debug MCU Configuration Register
    // Bit 8: DBG_IWDG_STOP - Stop IWDG when core is halted

    DBGMCU_CR |= DBG_IWDG_STOP;
}