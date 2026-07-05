#ifndef INC_USART_H
#define INC_USART_H
#include "common.h"

#define USART1_BASE              (PERIPHERAL_APB2_BASE + 0x3800U)
#define USART2_BASE              (PERIPHERAL_APB1_BASE + 0x4400U)
#define USART3_BASE              (PERIPHERAL_APB1_BASE + 0x4800U)

// usart registry struct
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

// USART bit positions & masks
#define USART_CR1_UE             BIT(13)
#define USART_CR1_TE             BIT(3)
#define USART_CR1_RE             BIT(2)
#define USART_CR1_PCE            BIT(10)     // Parity control enable
#define USART_CR1_M              BIT(12)     // databits 0 : 8 data bits, 1 : 9 data bits

#define USART_SR_RXNE            BIT(5)      // RX not empty (data available)
#define USART_SR_TC              BIT(6)
#define USART_SR_TXE             BIT(7)      // TX empty (ready for next byte)

#define USART_DR_MASK            0x1FFU      // the first 8 bits

// CR1 word length definitions
#define USART_DATABITS_8         0x00
#define USART_DATABITS_9         BIT(12)

// USART Configuration struct
typedef struct {
    uint32_t mode;          // USART_CR1_TE | USART_CR!_RE
    uint32_t databits;      // 0 for 8-bit, USART_CR1_M for 9-bit
    uint32_t parity;        // 0 for none, USART_CR1_PCE for enabled, etc
    uint32_t stopbits;      // 0 for 1 stop bit, USART_CR2_STOP1 for 2 bits
    uint32_t flow_control;
}usart_config_t;

// function prototypes
void usart_init(volatile usart_reg_t *usart, uint32_t baud_rate, const usart_config_t *config);
void usart_write_DR(volatile usart_reg_t *usart, uint16_t data);
void usart_write(volatile usart_reg_t *usart, const uint8_t *data, const uint32_t length);
uint16_t usart_read_DR(volatile usart_reg_t *usart);
bool usart_rx_available(volatile usart_reg_t *usart);

extern const usart_config_t USART1_TX_RX_8BIT;

#endif // INC_USART_H