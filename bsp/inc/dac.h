#ifndef INC_DAC_H   // DAC is not supported in bluepill
#define INC_DAC_H

#include "common.h"

#define DAC_BASE    (PERIPHERAL_APB1_BASE + 0x7400U)

typedef struct {
    volatile uint32_t CR;                                           // Control register
    volatile uint32_t SWTRIGR;                                      // Software trigger register
    volatile uint32_t DHR12R1;                                      // Channel 1 12-bit right-aligned data
    volatile uint32_t DHR12L1;                                      // Channel 1 12-bit left-aligned data
    volatile uint32_t DHR8R1;                                       // Channel 1 8-bit right-aligned data
    volatile uint32_t DHR12R2;                                      // Channel 2 12-bit right-aligned data
    volatile uint32_t DHR12L2;                                      // Channel 2 12-bit left-aligned data
    volatile uint32_t DHR8R2;                                       // Channel 2 8-bit right-aligned data
    volatile uint32_t DHR12RD;                                      // Dual DAC 12-bit right-aligned data
    volatile uint32_t DHR12LD;                                      // Dual DAC 12-bit left-aligned data
    volatile uint32_t DHR8RD;                                       // Dual DAC 8-bit right-aligned data
    volatile uint32_t DOR1;                                         // Channel 1 data output register
    volatile uint32_t DOR2;                                         // Channel 2 data output register
} DAC_reg_t;

#define DAC     ((volatile DAC_reg_t *) DAC_BASE)

#endif // INC_DAC_H