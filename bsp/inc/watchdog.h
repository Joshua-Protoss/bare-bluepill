#ifndef INC_WATCHDOG_H
#define INC_WATCHDOG_H

#include "common.h"

#define IWDG_BASE                           (PERIPHERAL_APB1_BASE + 0x3000U)
#define WWDG_BASE                           (PERIPHERAL_APB1_BASE + 0x2C00U)
#define DBGMCU_CR                           REG32(0xE0042004U)

// Debug bits
#define DBG_IWDG_STOP                       BIT(8)
#define DBG_WWDG_STOP                       BIT(9)

typedef struct {
    volatile uint32_t KR;              // Key register
    volatile uint32_t PR;              // Prescaler register
    volatile uint32_t RLR;             // Reload register
    volatile uint32_t SR;              // Status register
} IWDG_reg_t;

typedef struct {
    volatile uint32_t CR;               // Control register
    volatile uint32_t CFR;              // Configuration register
    volatile uint32_t SR;               // Status register
} WWDG_reg_t;

#define IWDG                                ((volatile IWDG_reg_t *) IWDG_BASE)
#define WWDG                                ((volatile WWDG_reg_t *) WWDG_BASE)

// IWDG Key values
#define IWDG_KR_KEY_ENABLE                  (0xCCCCU)
#define IWDG_KR_KEY_RELOAD                  (0xAAAAU)        // Kick the dog?
#define IWDG_KR_KEY_ACCESS                  (0x5555U)        // Unlock PR/RLR registers

// WWDG_CR bits
#define WWDG_CR_WDGA                        BIT(7)          // Activation bit
#define WWDG_CR_T_SHIFT                     0
#define WWDG_CR_T_MASK                      (0x7F << 0)     // 7-bit counter

// WWDG_CFR bits 
#define WWDG_CFR_W_SHIFT                    0
#define WWDG_CFR_W_MASK                     (0x7F << 0)     // 7-bit window value
#define WWDG_CFR_WDGTB_SHIFT                7
#define WWDG_CFR_WDGTB_MASK                 (0x03 << 7)
#define WWDG_CFR_EWI                        BIT(9)          // Early wakeup interrupt

// WWDG_SR bits
#define WWDG_SR_EWIF                        BIT(0)          // Early wakeup interrupt flag

// IWDG Prescaler values (40kHz LSI clock)
typedef enum {
    IWDG_PR_DIV4        = 0x00,                             // 10kHz,  max ~26.2s
    IWDG_PR_DIV8        = 0x01,                             // 5kHz,   max ~52.4s
    IWDG_PR_DIV16       = 0x02,                             // 2.5kHz, max ~104.9s
    IWDG_PR_DIV32       = 0x03,                             // 1.25kHz, max ~209.7s
    IWDG_PR_DIV64       = 0x04,                             // 625Hz,  max ~419.4s 
    IWDG_PR_DIV128      = 0x05,                             // 312.5Hz, max ~838.9s
    IWDG_PR_DIV256      = 0x06,                             // 156.25Hz, max ~1677.7s
}IWDG_prescaler_t;

// IWDG_clock = 40000 / (div64) =  625Hz
// Timeout period = (1/Watchdog clock) x reload value = (1 / 625) x 625 = 1 second

// WWDG Prescaler values
typedef enum {
    WWDG_PRESCALER_1    = (0x00 << 7),                      // APB1 / 4096
    WWDG_PRESCALER_2    = (0x01 << 7),                      // APB1 / 8192
    WWDG_PRESCALER_4    = (0x02 << 7),                      // APB1 / 16384
    WWDG_PRESCALER_8    = (0x03 << 7),                      // APB1 / 32768
} WWDG_prescaler_t;

// look at RM0008 section 20.4
// WWDG_timeout = APB1_clock_period(ms) x 4096 x 2^(WDGTB[1:0]) x (T[5:0] + 1)

void iwdg_init(IWDG_prescaler_t prescaler, uint16_t reload);
void iwdg_kick(void);
void iwdg_freeze_in_debug(void);                            // freeze during debugging

void wwdg_init(WWDG_prescaler_t prescaler, uint8_t reload, uint8_t window);
void wwdg_kick(uint8_t counter_value);
void wwdg_enable_ewi(void);                                // Early wakeup interrupt

#endif