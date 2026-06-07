#include "common.h"
#include "gpio.h"

void gpio_set_mode(GPIO_reg_t *gpio_port, GPIO_pin_t pin, GPIO_mode_t mode, GPIO_cnf_t cnf){
    uint32_t cr_reg;
    uint32_t shift;
    uint32_t config;

    // Determine which CR register to use 
    if (pin < 8) {
        cr_reg = gpio_port->CRL; // pins 0-7
        shift = pin * 4; // 4 bits: 2 lower bits for mode and 2 upper bits for cnf
    } else {
        cr_reg = gpio_port->CRH; // pins 8-15
        shift = (pin - 8) * 4;
    }

    // Combine mode and cnf
    config = ((cnf & 0x03) << 2) | (mode & 0x03);

    // Clear the 4 bits and set new configuration
    cr_reg &= ~(config << shift);
    cr_reg |= (config << shift);

    // Write to the appropriate register
    if (pin < 8){
        gpio_port->CRL = cr_reg;
    } else {
        gpio_port->CRH = cr_reg;
    }
}

void gpio_set_mode_mask(GPIO_reg_t *gpio_port, uint16_t pin_mask, GPIO_mode_t mode, GPIO_cnf_t cnf){
    for (uint8_t i  = 0; i < 16; i++){
        if (pin_mask & BIT(i)) {
            gpio_set_mode(gpio_port, i, mode, cnf);
        }
    }

}

void gpio_set_pull(GPIO_reg_t * gpio_reg, GPIO_pin_t pin, GPIO_pull_t pull){
    gpio_set_mode(gpio_reg, pin, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULLUPDOWN);

    // Set ODR bit for pull-up, clear for pull-down
    if (pull == GPIO_PULL_UP){
        gpio_reg->ODR |= BIT(pin);
    } else if (pull == GPIO_PULL_DOWN) {
        gpio_reg->ODR &= ~BIT(pin);
    }
}