#ifndef INC_RCC_H
#define INC_RCC_H

#include "common.h"

#define RCC_BASE                                (0x40021000U)

// Helper Macros
#define RCC_ENCODE(offset, bit_enable)          ((offset) << 5 | (bit_enable))
#define RCC_REG_ADDR(encoded)                   REG32(RCC_BASE + ((encoded) >> 5))
#define RCC_BIT_MASK(encoded)                   BIT((encoded) & 0x1f)

// registers offsets (STM32F103)
#define RCC_AHB_ENR_OFFSET       0x14
#define RCC_APB1_ENR_OFFSET      0x1c
#define RCC_APB2_ENR_OFFSET      0x18

// Bit positions for AHB peripherals
#define RCC_DMA1_BIT             0
#define RCC_DMA2_BIT             1

// Bit positions for APB2 peripherals
#define RCC_GPIOA_BIT            2      // GPIOA
#define RCC_GPIOB_BIT            3      // GPIOB
#define RCC_GPIOC_BIT            4      // GPIOC
#define RCC_GPIOD_BIT            5      // GPIOD
#define RCC_GPIOE_BIT            6      // GPIOE
#define RCC_GPIOF_BIT            7      // GPIOF
#define RCC_GPIOG_BIT            8      // GPIOG

// Bit positions for APB1 peripherals
#define RCC_TIM2_BIT             0
#define RCC_TIM3_BIT             1        
#define RCC_USART2_BIT           17

// encoded clock enable values
enum rcc_periph_clken {
    // AHB peripherals
    RCC_DMA1 = RCC_ENCODE(RCC_AHB_ENR_OFFSET, RCC_DMA1_BIT),
    RCC_DMA2 = RCC_ENCODE(RCC_AHB_ENR_OFFSET, RCC_DMA2_BIT),

    // APB1 peripherals
    RCC_TIM2 = RCC_ENCODE(RCC_APB1_ENR_OFFSET, RCC_TIM2_BIT),
    RCC_TIM3 = RCC_ENCODE(RCC_APB1_ENR_OFFSET, RCC_TIM3_BIT),
    RCC_USART2 = RCC_ENCODE(RCC_APB1_ENR_OFFSET, RCC_USART2_BIT),

    // APB2 peripherals
    RCC_GPIOA = RCC_ENCODE(RCC_APB2_ENR_OFFSET, RCC_GPIOA_BIT),
    RCC_GPIOB = RCC_ENCODE(RCC_APB2_ENR_OFFSET, RCC_GPIOB_BIT),
    RCC_GPIOC = RCC_ENCODE(RCC_APB2_ENR_OFFSET, RCC_GPIOC_BIT),
    RCC_GPIOD = RCC_ENCODE(RCC_APB2_ENR_OFFSET, RCC_GPIOD_BIT),
    RCC_GPIOE = RCC_ENCODE(RCC_APB2_ENR_OFFSET, RCC_GPIOE_BIT),
    RCC_GPIOF = RCC_ENCODE(RCC_APB2_ENR_OFFSET, RCC_GPIOF_BIT),
    RCC_GPIOG = RCC_ENCODE(RCC_APB2_ENR_OFFSET, RCC_GPIOG_BIT),
 
};

void rcc_periph_clock_enable(enum rcc_periph_clken encoded_clken);
void rcc_periph_clock_disable(enum rcc_periph_clken encoded_clken);

#endif // INC_RCC_H