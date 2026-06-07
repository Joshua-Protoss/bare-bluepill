#ifndef INC_GPIO_H
#define INC_GPIO_H

#include "common.h"

// GPIO register structure
typedef struct GPIO_reg_t{
    uint32_t CRL;
    uint32_t CRH;
    uint32_t IDR;
    uint32_t ODR;
    uint32_t BSRR;
    uint32_t BRR;
    uint32_t LCKR;
} GPIO_reg_t;

// Mode structure
typedef enum {
    GPIO_MODE_INPUT = 0x00,
    GPIO_MODE_OUTPUT_10MHZ = 0x01,
    GPIO_MODE_OUTPUT_2MHZ = 0x02,
    GPIO_MODE_OUTPUT_50MHZ = 0x03,
}GPIO_mode_t;

// CNF structure
typedef enum {
    // input mode
    GPIO_CNF_INPUT_ANALOG = 0x00,
    GPIO_CNF_INPUT_FLOATING = 0x01,
    GPIO_CNF_INPUT_PULLUPDOWN = 0x02,
    // output mode
    GPIO_CNF_OUTPUT_PUSHPULL = 0x00,
    GPIO_CNF_OUTPUT_OPENDRAIN = 0x01,
    GPIO_CNF_OUTPUT_AF_PUSHPULL = 0x02,
    GPIO_CNF_OUTPUT_AF_OPENDRAIN = 0x03,
}GPIO_cnf_t;

// Pin structure
typedef enum{
    PIN_GPIO0 = 0,
    PIN_GPIO1 = 1,
    PIN_GPIO2 = 2,
    PIN_GPIO3 = 3,
    PIN_GPIO4 = 4,
    PIN_GPIO5 = 5,
    PIN_GPIO6 = 6,
    PIN_GPIO7 = 7,
    PIN_GPIO8 = 8,
    PIN_GPIO9 = 9,
    PIN_GPIO10 = 10,
    PIN_GPIO11 = 11,
    PIN_GPIO12 = 12,
    PIN_GPIO13 = 13,
    PIN_GPIO14 = 14,
    PIN_GPIO15 = 15
}GPIO_pin_t;

typedef enum {
    GPIO_PULL_NONE,
    GPIO_PULL_UP,
    GPIO_PULL_DOWN
}GPIO_pull_t;

// GPIO base addresses from section 3.3 memory map stm32 reference manual
#define PORT_GPIOA ((GPIO_reg_t*)(PERIPHERAL_APB2_BASE + 0x0800U))
#define PORT_GPIOB ((GPIO_reg_t*)(PERIPHERAL_APB2_BASE + 0x0c00U))
#define PORT_GPIOC ((GPIO_reg_t*)(PERIPHERAL_APB2_BASE + 0x1000U))
#define PORT_GPIOD ((GPIO_reg_t*)(PERIPHERAL_APB2_BASE + 0x1400U))
#define PORT_GPIOE ((GPIO_reg_t*)(PERIPHERAL_APB2_BASE + 0x1800U))
#define PORT_GPIOF ((GPIO_reg_t*)(PERIPHERAL_APB2_BASE + 0x1c00U))
#define PORT_GPIOG ((GPIO_reg_t*)(PERIPHERAL_APB2_BASE + 0x2000U))

// function prototypes
void gpio_set_mode(GPIO_reg_t *gpio_port, GPIO_pin_t pin, GPIO_mode_t mode, GPIO_cnf_t cnf);
void gpio_set_mode_mask(GPIO_reg_t *gpio_port, uint16_t pin_mask, GPIO_mode_t mode, GPIO_cnf_t cnf);
void gpio_set_pull(GPIO_reg_t *gpio_port, GPIO_pin_t pin, GPIO_pull_t pull);

// write pin
static inline void gpio_write_pin(GPIO_reg_t *gpio_port, GPIO_pin_t pin, bool value) {
    if (value){
        gpio_port->BSRR = BIT(pin);
    }else{
        gpio_port->BRR = BIT(pin);
    }
}

// read pin
static inline bool gpio_read_pin(GPIO_reg_t *gpio_port, GPIO_pin_t pin){
    return (gpio_port->IDR >> pin) & 1;
}

// toggle pin
static inline void gpio_toggle_pin(GPIO_reg_t *gpio_port, GPIO_pin_t pin){
    if(gpio_read_pin(gpio_port, pin)){
        gpio_write_pin(gpio_port, pin, false);
    } else {
        gpio_write_pin(gpio_port, pin, true);
    }
}


#endif //INC_GPIO_H