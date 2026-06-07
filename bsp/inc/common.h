#ifndef INC_COMMON_H
#define INC_COMMON_H

#include <stdint.h>
#include <stdbool.h>

// Memory map for all busses
#define FLASH_BASE                          (0x08000000U)
#define PERIPHERAL_BASE                     (0x40000000U)
#define PERIPHERAL_APB1_BASE                (PERIPHERAL_BASE + 0x00000U)
#define PERIPHERAL_APB2_BASE                (PERIPHERAL_BASE + 0x10000U)
#define PERIPHERAL_AHB1_BASE                (PERIPHERAL_BASE + 0x18000U)


#define BIT(n) (1UL << (n)) // Safer for 32-bit registers
#define REG32(addr) (*(volatile uint32_t *)(addr))

#endif // include common.h