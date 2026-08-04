#include "faultlog.h"
#include "rcc.h"
#include "usart.h"
#include "watchdog.h"

// BKP register definitions
#define BKP_BASE                            (PERIPHERAL_APB1_BASE + 0x6C00U)

// BKP register struct
typedef struct {
    uint32_t _reserved1;
    volatile uint32_t DR1;
    volatile uint32_t DR2;
    volatile uint32_t DR3;
    volatile uint32_t DR4;
    volatile uint32_t DR5;
    volatile uint32_t DR6;
    volatile uint32_t DR7;
    volatile uint32_t DR8;
    volatile uint32_t DR9;
    volatile uint32_t DR10;
    volatile uint32_t RTCCR;
    volatile uint32_t CR;
    volatile uint32_t CSR;
    uint32_t _reserved2[2];
    volatile uint32_t DR11;
} BKP_reg_t;

#define BKP                                 ((volatile BKP_reg_t *)BKP_BASE)

// PWR register for backup access
#define PWR_BASE                            (PERIPHERAL_APB1_BASE + 0x7000U)

// PWR register struct
typedef struct {
    volatile uint32_t CR;
    volatile uint32_t CSR;
} PWR_reg_t;

#define PWR                                 ((volatile PWR_reg_t *)PWR_BASE)
#define PWR_CR_DBP                          BIT(8)  // Disable Backup Protection

void fault_log_init(void) {
    // Enable clocks
    RCC->APB1ENR |= RCC_PWREN | RCC_BKPEN;                  // note: RCC_PWREN = BIT(27), RCC_BKPEN = BIT(28)
    
    // Enable backup register access
    PWR->CR |= PWR_CR_DBP;
    
    // If magic number isn't set, initialize backup registers
    if (BKP->DR8 != BKP_MAGIC_VALID) {
        BKP->DR1 = 0;
        BKP->DR2 = 0;
        BKP->DR3 = STATE_INIT;
        BKP->DR4 = 0;
        BKP->DR5 = 0;
        BKP->DR6 = 0;
        BKP->DR7 = 0;
        BKP->DR8 = BKP_MAGIC_VALID;
        BKP->DR9 = 0;
        BKP->DR10 = 0;
    }
}

void fault_log_record(fault_code_t fault, system_state_t state, 
                      uint16_t extra1, uint16_t extra2) {
    // Atomic recording - disable interrupts if needed
    //__disable_irq();
    
    BKP->DR1 = (uint32_t)fault;
    //BKP->DR2 = (uint32_t)systick_ticks;  // Timestamp
    BKP->DR3 = (uint32_t)state;
    BKP->DR4 = extra1;                    // Phase current or bus voltage
    BKP->DR5 = extra2;                    // Additional data
    BKP->DR6 = __get_MSP();              // Current stack pointer
    BKP->DR7 = BKP->DR7 + 1;             // Increment fault counter
    BKP->DR8 = BKP_MAGIC_VALID;          // Re-validate
    BKP->DR9 = WWDG->CR & 0x7F;          // WWDG counter snapshot
    
   // __enable_irq();
}

void fault_log_dump(void) {
    if (!fault_log_is_valid()) {
        usart_printf(USART1, "No valid fault log found\r\n");
        return;
    }
    
    usart_printf(USART1, "\r\n========== FAULT LOG ==========\r\n");
    usart_printf(USART1, "Fault Code:    0x%04lX\r\n", BKP->DR1);
    usart_printf(USART1, "Timestamp:     %lu ms\r\n", BKP->DR2);
    usart_printf(USART1, "System State:  0x%02lX\r\n", BKP->DR3);
    usart_printf(USART1, "Extra Data 1:  %lu\r\n", BKP->DR4);
    usart_printf(USART1, "Extra Data 2:  %lu\r\n", BKP->DR5);
    usart_printf(USART1, "Stack Pointer: 0x%08lX\r\n", BKP->DR6);
    usart_printf(USART1, "Fault Count:   %lu\r\n", BKP->DR7);
    usart_printf(USART1, "WWDG Counter:  %lu\r\n", BKP->DR9);
    usart_printf(USART1, "===============================\r\n");
    
    // Show fault description
    switch (BKP->DR1) {
        case FAULT_WWDG_TIMING:
            usart_printf(USART1, "Cause: FOC control loop timing violation\r\n");
            break;
        case FAULT_OVERCURRENT_PHASE_A:
            usart_printf(USART1, "Cause: Phase A overcurrent\r\n");
            break;
        // ... etc
    }
}

void fault_log_clear(void) {
    BKP->DR1 = 0;
    BKP->DR2 = 0;
    BKP->DR3 = STATE_INIT;
    BKP->DR4 = 0;
    BKP->DR5 = 0;
    BKP->DR6 = 0;
    BKP->DR8 = BKP_MAGIC_VALID;  // Keep magic but clear fault
    BKP->DR9 = 0;
}

bool fault_log_is_valid(void) {
    return (BKP->DR8 == BKP_MAGIC_VALID) && (BKP->DR1 != 0);
}