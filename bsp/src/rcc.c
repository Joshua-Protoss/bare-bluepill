#include "rcc.h"

void rcc_periph_clock_enable(enum rcc_periph_clken encoded_clken){
    RCC_REG_ADDR(encoded_clken) |= RCC_BIT_MASK(encoded_clken);
}

void rcc_periph_clock_disable(enum rcc_periph_clken encoded_clken){
    RCC_REG_ADDR(encoded_clken) &= ~RCC_BIT_MASK(encoded_clken);
}