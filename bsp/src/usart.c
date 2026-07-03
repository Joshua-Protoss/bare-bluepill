#include "usart.h"
#include "rcc.h"

void usart_init(volatile usart_reg_t *usart, uint32_t baud_rate, const usart_config_t *config){

    // calculate apb_clock
    uint32_t apb_clock = (usart == USART1) ? rcc_get_apb2_freq() : rcc_get_apb1_freq();
    usart->BRR = (apb_clock + (baud_rate / 2)) / baud_rate;

    // Configure stopbits at CR2
    usart->CR2 = config->stopbits;

    // Configure flow control at CR3
    usart->CR3 = config->flow_control;

    // Enable USART, Transmitter, and Receiver
    usart->CR1 = USART_CR1_UE | config->mode | config->databits | config->parity;
}

void usart_send_char(volatile usart_reg_t *usart, uint16_t data) {
    while(!(usart->SR & USART_SR_TXE)); // Wait for TX buffer empty
    usart->DR = data;
}

const usart_config_t USART1_TX_RX_8 = {
    .mode = USART_CR1_TE | USART_CR1_RE,
    .databits = 0x00,
    .parity = 0x00,
    .stopbits = 0x00,
    .flow_control = 0x00,
};