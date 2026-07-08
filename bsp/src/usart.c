#include "usart.h"
#include "rcc.h"
#include "nvic.h"

void usart_init(volatile usart_reg_t *usart, uint32_t baud_rate, const usart_config_t *config){

    // calculate apb_clock
    uint32_t apb_clock = (usart == USART1) ? rcc_get_apb2_freq() : rcc_get_apb1_freq();
    usart->BRR = (apb_clock + (baud_rate / 2)) / baud_rate;
    //usart->BRR = (44000000 / baud_rate); there was a problem with rcc_get_sysclk_freq()

    // Configure stopbits at CR2
    usart->CR2 = config->stopbits;

    // Configure flow control at CR3
    usart->CR3 = config->flow_control;

    // Enable USART, Transmitter, and Receiver
    usart->CR1 = USART_CR1_UE | config->mode | config->databits | config->parity;
}

void usart_write_DR(volatile usart_reg_t *usart, uint16_t data) {
    while(!(usart->SR & USART_SR_TXE));             // Wait for TX buffer empty
    usart->DR = data;
}

void usart_write(volatile usart_reg_t *usart, const uint8_t *data, const uint32_t length){  // change const to volatile
    for (uint32_t i = 0; i < length; i++){
        usart_write_DR(usart, (uint16_t) data[i]);
    }
}

uint16_t usart_read_DR(volatile usart_reg_t *usart){
    while(!(usart->SR & USART_SR_RXNE));           // Wait for RX buffer received data
    return usart->DR & USART_DR_MASK;              // read only the 16 bits of DR register
}

bool usart_rx_available(volatile usart_reg_t *usart) {
    return (usart->SR & USART_SR_RXNE) != 0;
}

void usart_write_string(volatile usart_reg_t *usart, const char *str){
    while(*str) {
        if (*str == '\n') {
            usart_write_DR(usart, '\r');         // Auto add CR before LF
        }
        usart_write_DR(usart, (uint16_t) *str++);
    }
}

void usart_rx_interrupt_enable(volatile usart_reg_t *usart){
    usart->CR1 |= USART_CR1_RXNEIE;         // enable usart rx interrupt
    
    // enable nvic
    if (usart == USART1) {
        nvic_enable_irq(NVIC_USART1_IRQ);
    } else if (usart == USART2) {
        nvic_enable_irq(NVIC_USART2_IRQ);
    } else if (usart == USART3) {
        nvic_enable_irq(NVIC_USART3_IRQ);
    }
}

void usart_rx_interrupt_disable(volatile usart_reg_t *usart){
    usart->CR1 &= ~USART_CR1_RXNEIE;

    // disable nvic
    if (usart == USART1) {
        nvic_disable_irq(NVIC_USART1_IRQ);
    } else if (usart == USART2) {
        nvic_disable_irq(NVIC_USART2_IRQ);
    } else if (usart == USART3) {
        nvic_disable_irq(NVIC_USART3_IRQ);
    }
}

void usart_rx_dma_enable(volatile usart_reg_t *usart){
    usart->CR3 |= USART_CR3_DMAR;
}

void usart_rx_dma_disable(volatile usart_reg_t *usart){
    usart->CR3 &= ~USART_CR3_DMAR;
}

const usart_config_t USART1_TX_RX_8BIT = {
    .mode = USART_CR1_TE | USART_CR1_RE,
    .databits = 0x00,
    .parity = 0x00,
    .stopbits = 0x00,
    .flow_control = 0x00,
};

const usart_config_t USART1_TX_RX_9BIT = {
    .mode = USART_CR1_TE | USART_CR1_RE,
    .databits = USART_DATABITS_9,
    .parity = 0x00,
    .stopbits = 0x00,
    .flow_control = 0x00,
};