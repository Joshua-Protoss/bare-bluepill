#ifndef INC_USART_H
#define INC_USART_H
#include "common.h"

#define USART1_BASE              (PERIPHERAL_APB2_BASE + 0x3800U)
#define USART2_BASE              (PERIPHERAL_APB1_BASE + 0x4400U)
#define USART3_BASE              (PERIPHERAL_APB1_BASE + 0x4800U)

typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;  
    volatile uint32_t GTPR;
}usart_reg_t;

#define USART1                   ((volatile usart_reg_t *) USART1_BASE)
#define USART2                   ((volatile usart_reg_t *) USART2_BASE)
#define USART3                   ((volatile usart_reg_t *) USART3_BASE)

// USART bit positions
#define USART_CR1_UE             BIT(13)
#define USART_CR1_TE             BIT(3)
#define USART_CR1_RE             BIT(2)

#define USART_SR_RXNE            BIT(5)   // RX not empty (data available)
#define USART_SR_TC              BIT(6)
#define USART_SR_TXE             BIT(7)   // TX empty (ready for next byte)

#endif // INC_USART_H