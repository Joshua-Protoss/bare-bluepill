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

#define IWDG                                ((volatile IWDG_reg_t *) IWDG_BASE)

// IWDG Key values
#define IWDG_KR_KEY_ENABLE                  (0xCCCC)
#define IWDG_KR_KEY_RELOAD                  (0xAAAA)        // Kick the dog?
#define IWDG_KR_KEY_ACCESS                  (0x5555)        // Unlock PR/RLR registers

// Prescaler values (40kHz LSI clock)
typedef enum {
    IWDG_PR_DIV4        = 0x00,                             // 10kHz,  max ~26.2s
    IWDG_PR_DIV8        = 0x01,                             // 5kHz,   max ~52.4s
    IWDG_PR_DIV16       = 0x02,                             // 2.5kHz, max ~104.9s
    IWDG_PR_DIV32       = 0x03,                             // 1.25kHz, max ~209.7s
    IWDG_PR_DIV64       = 0x04,                             // 625Hz,  max ~419.4s 
    IWDG_PR_DIV128      = 0x05,                             // 312.5Hz, max ~838.9s
    IWDG_PR_DIV256      = 0x06,                             // 156.25Hz, max ~1677.7s
}IWDG_prescaler_t;

// Watchdog clock = 40000 / (div64) =  625Hz
// Timeout period = (1/Watchdog clock) x reload value = (1 / 625) x 625 = 1 second

void iwdg_init(IWDG_prescaler_t prescaler, uint16_t reload);
void iwdg_kick(void);
void iwdg_freeze_in_debug(void);                            // freeze during debugging

#endif