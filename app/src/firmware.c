#include "common.h"
#include "rcc.h"
#include "gpio.h"
#include "systick.h"
#include <string.h>
#include "usart.h"
#include "timers.h"
#include "nvic.h"
#include "i2c.h"

#define SYSTICK_FREQ                    (1000)            // the desired systick frequency, 1000Hz means 1ms per tick  
#define USART_PORT                      (RCC_GPIOA)
#define USART_TX_PIN                    (PIN_GPIO9)       // PA9
#define USART_RX_PIN                    (PIN_GPIO10)      // PA10

// Test: Generate PWM on TIM2 CH1 (PA0), read it with TIM3 CH1 (PA6)
// Connect PA0 to PA6 with a jumper wire!

volatile uint32_t systick_ticks = 0;

// Pre-defined messages as uint8_t arrays
static const uint8_t msg_welcome[] = "\r\n=== I2C MAX30102 Test ===\r\n";
static const uint8_t msg_prompt[] = "\r\n>";

void systick_handler(void){
    systick_ticks++;
}

void uart_setup(){
    rcc_periph_clock_enable(USART_PORT);
    rcc_periph_clock_enable(RCC_USART1);
    // PA9 = TX (AF push-pull)
    gpio_set_mode(PORT_GPIOA, USART_TX_PIN, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
    // PA10 = RX (floating input)
    gpio_set_mode(PORT_GPIOA, USART_RX_PIN, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOATING);

    // Configure USART1
    usart_init(USART1, 115200, &USART1_TX_RX_8BIT);    // the function will automatically compute the correct BRR for 44MHz

    // startup messages
    usart_write(USART1, msg_welcome, sizeof(msg_welcome)-1);
    usart_write(USART1, msg_prompt, sizeof(msg_prompt)-1);
    usart_printf(USART1, "SysClk: %lu Hz\r\n", rcc_get_sysclk_freq());
    usart_printf(USART1, "APB1: %lu Hz\r\n", rcc_get_apb1_freq());
    usart_printf(USART1, "APB2: %lu Hz\r\n", rcc_get_apb2_freq());
}

void max30102_i2c_test(void) {
    i2c_bitbang_init(&MAX30102_I2C_CFG);

    // Probe the device
    usart_printf(USART1, "Probing 0x57...");
    if (i2c_probe(0x57)) {
        usart_printf(USART1, "Device found!\r\n");
    } else {
        usart_printf(USART1, "No response. Check wiring. \r\n");
        return;
    }

    // Read Part ID (register 0xFF)
    uint8_t part_id;
    if (i2c_read(0x57, 0xFF, &part_id, 1)) {                                         // "Set your register pointer to 0xFF (Part ID register)"
        usart_printf(USART1, "Part ID: 0x%02X ", part_id);                           // we will get data from the 0xFF (part id) register
        if (part_id == 0x15) {
            usart_printf(USART1, "MAX30102 detected! \r\n");
        } else {
            usart_printf(USART1, "Unknown, expected 0x15 \r\n");
        }
    } else {
        usart_printf(USART1, "Failed to read Part ID \r\n");
    }

    // Read Revision ID (register 0xFE)
    uint8_t rev_id;
    if (i2c_read(0x57, 0xFE, &rev_id, 1)) {
        usart_printf(USART1, "Revision ID: 0x%02X\r\n", rev_id);
    } else {
        usart_printf(USART1, "Failed to read Revision ID \r\n");
    }

    usart_printf(USART1, "=== Test Complete ===\r\n");
}

void i2c_scan_bus(void) {
    usart_printf(USART1, "\r\n=== I2C Bus Scan ===\r\n");
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        if (i2c_probe(addr)) {
            usart_printf(USART1, "Device found at 0x%02X!\r\n", addr);
        }
    }
    usart_printf(USART1, "=== Scan Complete ===\r\n");
}

void i2c_hardware_test(void) {
    uint8_t part_id, rev_id, temp;
    usart_printf(USART1, "\r\n=== Hardware I2C Test ===\r\n");

    // Initialize hardware I2C at 100kHz
    i2c_hardware_init(100000);
    systick_delay_ms(10);
    // Probe device
    usart_printf(USART1, "Hardware Probe 0x57...");
    if (i2c_hardware_probe(0x57)) {
        usart_printf(USART1, "Device found! \r\n");
    } else {
        usart_printf(USART1, "No response! \r\n");
        return;
    }

    // Read Part ID
    if (i2c_hardware_read(0x57, 0xFF, &part_id, 1)) {
        usart_printf(USART1, "Part ID: 0x%02X\r\n", part_id);
    }

    // Read Revision ID
    if (i2c_hardware_read(0x57, 0xFE, &rev_id, 1)) {
        usart_printf(USART1, "Rev ID: 0x%02X\r\n", rev_id);
    }

    // Read Temperature
    if (i2c_hardware_read(0x57, 0x1F, &temp, 1)) {
        usart_printf(USART1, "temperature: %d°C\r\n", (int8_t)temp);
    }
}

void test_hardware_i2c_minimal(void) {
    // Enable clocks
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_I2C1);
    systick_delay_ms(10);
    
    // Reset I2C
    I2C1->CR1 = 0x8000;  // SWRST
    systick_delay_ms(10);
    I2C1->CR1 = 0x0000;
    systick_delay_ms(10);
    
    PORT_GPIOB->CRL &= ~(0xFF << 24);  // Clear PB6, PB7
    PORT_GPIOB->CRL |= (0x88 << 24);   // Input pull-up/down for PB6, PB7
    PORT_GPIOB->ODR |= (0x03 << 6);    // Enable pull-UP (ODR=1)
    systick_delay_ms(10);
    // Switch to AF Open-Drain (pull-up stays active!)
    PORT_GPIOB->CRL &= ~(0xFF << 24);  // Clear PB6, PB7
    PORT_GPIOB->CRL |= (0x77 << 24);   // AF Open-Drain 50MHz
    
    // Configure I2C
    I2C1->CR2 = 22;
    I2C1->CCR = 110;
    I2C1->TRISE = 23;
    
    // Enable
    I2C1->CR1 = 0x0001;  // PE
    systick_delay_ms(10);

    // Check if lines are HIGH
    usart_printf(USART1, "SCL=%d SDA=%d\r\n",
                 (PORT_GPIOB->IDR >> 6) & 1,
                 (PORT_GPIOB->IDR >> 7) & 1);
    
    // Test: Generate START
    I2C1->CR1 |= 0x0100;  // START
    
    uint32_t timeout = 100000;
    while(!(I2C1->SR1 & 0x0001) && --timeout);              // Wait SB
    
    if (timeout == 0) {
        usart_printf(USART1, "SB timeout!\r\n");
        return;
    }
    
    usart_printf(USART1, "SB detected! Sending address...\r\n");
    
    // Send address
    I2C1->DR = 0xAE;  // 0x57 << 1 | 0
    
    timeout = 100000;
    while(--timeout) {
        if (I2C1->SR1 & 0x0002) {  // ADDR
            usart_printf(USART1, "ADDR received - device ACKed!\r\n");
            (void)I2C1->SR2;
            I2C1->CR1 |= 0x0200;  // STOP
            return;
        }
        if (I2C1->SR1 & 0x0400) {  // AF
            usart_printf(USART1, "AF - no device!\r\n");
            I2C1->CR1 |= 0x0200;  // STOP
            return;
        }
    }
    
    usart_printf(USART1, "Timeout!\r\n");
    I2C1->CR1 |= 0x0200;  // STOP
}

int main(void) {
    rcc_clock_configure(&RCC_CLOCK_HSE_44MHZ);
    systick_set_frequency(SYSTICK_FREQ, rcc_get_ahb_freq());        // 1ms tick, interrupt enabled by default
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOB);
    gpio_set_mode(PORT_GPIOC, PIN_GPIO13, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_PUSHPULL);
    uart_setup();
    
    usart_printf(USART1, "APB1 Timer Clock: %lu Hz\r\n", rcc_get_apb1_timer_freq());
    usart_printf(USART1, "Expected PWM period: %lu ticks @ 1kHz\r\n", rcc_get_apb1_timer_freq() / 1000);
    //max30102_i2c_test();
    //i2c_scan_bus();
    // i2c_sensor_init();
    // // Verify FIFO has data with bit-bang
    // usart_printf(USART1, "\r\n=== Bit-Bang FIFO Test ===\r\n");
    // for (int i = 0; i < 5; i++) {
    //     i2c_bitbang_fifo();
    //     systick_delay_ms(100);
    // }
    usart_printf(USART1, "\r\n=== Hardware FIFO Test ===\r\n");
    test_hardware_i2c_minimal();
    i2c_hardware_sensor_init();
    
    while(1){
        uint8_t fifo_data[32];
        if (i2c_hardware_read_fifo(0x57, fifo_data, 6)) {
            uint32_t ir = (fifo_data[0] << 16) | (fifo_data[1] << 8) | fifo_data[2];
            ir &= 0x3FFFF;
            // Parse second sample (bytes 3-5)
            uint32_t red = (fifo_data[3] << 16) | (fifo_data[4] << 8) | fifo_data[5];
            red &= 0x3FFFF;

            usart_printf(USART1, "IR: %lu, Red: %lu \r\n", ir, red);
        }
        systick_delay_ms(200);

        __asm__("wfi");  // Sleep, save power!
    }
       

    return 0;
}

