#ifndef INC_SYSTICK_H
#define INC_SYSTICK_H

#include "common.h"

// Offsets and peripheral memory addresses
#define SYSTICK_BASE                             (0xE000E010U)
#define SYSTICK_RELOAD_MAX                       (0x00FFFFFFU)
#define SYSTICK_CURRENT_VALUE_MASK               (0x00FFFFFFU)
#define SYSTICK_CALIB_TENMS                      (0x00FFFFFFU)


// Systick register map
typedef struct {
    volatile uint32_t SYST_CSR;      // SysTick control and status register, STK_CTRL in STM32
    volatile uint32_t SYST_RVR;      // SysTick reload value register, STK_LOAD in STM32
    volatile uint32_t SYST_CVR;      // SysTick current value register, STK_VAL in STM32
    volatile uint32_t SYST_CALIB;    // SysTick calibration value register, STK_CALIB in STM32
} Systick_reg_t;

#define SYSTICK                                     ((Systick_reg_t *) SYSTICK_BASE) // Systick register

// Systick control and status register (STK_CTR in STM32, CSR in ARM) bit shift
#define SYST_CSR_ENABLE                              BIT(0)  // 1 = Enable counter
#define SYST_CSR_TICKINT                             BIT(1)  // 1 = Enable interrupt on count to 0
#define SYST_CSR_CLKSOURCE                           BIT(2)  // 1 = CPU clock, 0 = AHB/8
#define SYST_CSR_COUNTFLAG                           BIT(16) // Returns 1 if timer counted to 0 since last time this was read.

// Function prototypes
bool systick_set_frequency(uint32_t desired_freq, uint32_t ahb_freq);
void systick_set_reload(uint32_t reload);
uint32_t systick_get_reload(void);
uint32_t systick_get_value(void);
void systick_timer_enable(void);
void systick_timer_disable(void);
void systick_interrupt_enable(void);
void systick_interrupt_disable(void);
bool systick_get_countflag(void);
void systick_clear_counter(void);
uint32_t systick_get_calib(void);

#endif // INC_SYSTICK_H