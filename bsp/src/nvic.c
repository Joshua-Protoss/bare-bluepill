#include "nvic.h"

void nvic_enable_irq(uint8_t irq_number){
    if (irq_number < 32) {
        NVIC_ISER0 = BIT(irq_number);
    } else {
        NVIC_ISER1 = BIT(irq_number - 32);
    }
}

void nvic_disable_irq(uint8_t irq_number){
    if (irq_number < 32) {
        NVIC_ICER0 = BIT(irq_number);
    } else {
        NVIC_ICER1 = BIT(irq_number - 32);
    }
}

void nvic_set_priority(uint8_t irq_number, uint8_t priority){
    // STM32F103 uses upper 4 bits of each priority byte
    volatile uint8_t *ipr = (volatile uint8_t *) (NVIC_IPR_BASE + irq_number);
    *ipr = (priority & 0x0F) << 4;   // Safely mask to 4-bits, shift to the upper nibble, and write
}