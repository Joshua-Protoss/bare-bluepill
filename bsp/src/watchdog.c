#include "watchdog.h"

void iwdg_init(IWDG_prescaler_t prescaler, uint16_t reload) {
    // Unlock PR and RLR registers
    IWDG->KR = IWDG_KR_KEY_ACCESS;

    // Set prescaler
    IWDG->PR = prescaler;

    // Set reload value (12-bit: 0-4095)
    IWDG->RLR = reload & 0xFFFU;

    // Wait for registers to be updated
    while (IWDG->SR & BIT(0));                      // Wait for PVU
    while (IWDG->SR & BIT(1));                      // Wait for RVU

    // Start the watchdog
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