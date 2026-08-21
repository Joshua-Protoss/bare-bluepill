#include "i2c.h"
#include "rcc.h"
#include "systick.h"
#include "usart.h"

// Current configuration
static I2C_config_t current_config;
static bool initialized = false;

// ===== LOW-LEVEL GPIO CONTROL =====
static inline void scl_low(void) {
    gpio_write_pin(current_config.port, current_config.scl_pin, 0);
}

static inline void scl_high(void) {
    gpio_write_pin(current_config.port, current_config.scl_pin, 1);
}

static inline void sda_low(void) {                                       // For open-drain, set pin LOW (output mode drives low)
    gpio_set_mode(current_config.port, current_config.sda_pin,
                  GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_PUSHPULL);
    gpio_write_pin(current_config.port, current_config.sda_pin, 0);      // First, ensure pin is in output mode
}

static inline void sda_high (void) {                                     // For open-drain, "release" the pin (set to input/floating)
    gpio_set_mode(current_config.port, current_config.sda_pin,           // The pull-up resistor will pull it HIGH
              GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOATING);
}

static inline uint8_t sda_read(void) {
    return gpio_read_pin(current_config.port, current_config.sda_pin);
}

// ===== DELAY FUNCTIONS =====                                            // For 100kHz: bit period = 10µs, half = 5µs
static void i2c_delay_half_bit(void) {                                    // For 400kHz: bit period = 2.5µs, half = 1.25µs                   
    for (volatile uint32_t i = 0; i < 220; i++) {                         // ~5µs delay for 100kHz
        __asm__("nop");
    }
}

// ===== I2C SIGNAL GENERATION =====
static void i2c_start(void) {                                          // START: SDA goes LOW while SCL is HIGH
    sda_high();                                                        // sda_high() and scl_high() is the idle state
    scl_high();                                                        // The I2C protocol dictates that data on the SDA line 
    i2c_delay_half_bit();                                              // can only change while SCL is LOW.
    sda_low();                                                         // A split-second later, the Master pulls SCL LOW.
    i2c_delay_half_bit();                                              // Every slave on the bus sees this specific transition
    scl_low();                                                         // and resets its internal bit counters.
    i2c_delay_half_bit();
}

static void i2c_stop(void) {                                           // STOP: SDA goes HIGH while SCL is HIGH
    sda_low();                                                         // While SCL is LOW, the Master pulls SDA LOW
    scl_high();                                                        // The Master releases SCL HIGH
    i2c_delay_half_bit();                                              // While SCL remains HIGH, the Master releases SDA HIGH.
    sda_high();                                                        // The bus is now back in its Idle State (both lines HIGH)
    i2c_delay_half_bit();                                              // Waiting for the next START condition.
}

static uint8_t i2c_write_byte(uint8_t data) {                          // Send 8 bits, MSB first
    for (uint8_t i = 0; i < 8; i++) {                                  // While SCL is LOW, the Master changes SDA 
        if (data & 0x80) {                                             // to match the data bit (1 = float/high, 0 = pull low).
            sda_high();                                                // The I2C protocol dictates that data on the SDA line 
        } else {                                                       // can only change while SCL is LOW.
            sda_low();
        }
        i2c_delay_half_bit();                                          // The Master drives SCL HIGH. The lines freeze. 
        scl_high();                                                    // Every slave reads the SDA state right now.
        i2c_delay_half_bit();                                          // Prepare Next Bit
        scl_low();                                                     // The Master drives SCL LOW again.
        data <<= 1;                                                    // Send the next bit
    }

    // The 9th Clock Pulse: The ACK/NACK Check                         // See if a slave actually exists at that address.
    sda_high();                                                        // Release SDA for slave to drive, slave will set SDA low
    i2c_delay_half_bit();                                              // If a slave matches the address: 
    scl_high();                                                        // The slave actively pulls SDA LOW (ACK = 0) while SCL is still LOW
    i2c_delay_half_bit();                                              // If no slave exists: The line stays HIGH via the resistors (NACK = 1)
    uint8_t ack = sda_read();                                          // The Master set the SCL HIGH before reading this pin state,     
    scl_low();                                                         // then drives SCL LOW to conclude the address phase.
    i2c_delay_half_bit();                                              // 0 = ACK, 1 = NACK
    return ack;                                                        // 0 = success, 1 = failure
}

static uint8_t i2c_read_byte(bool send_ack) {                          // Release SDA for slave to drive with sda_high() 
    uint8_t data = 0;                                                  // right before sda_read() is mandatory because it                                         
    sda_high();                                                        // Physically steps the Master off the SDA line so the slave
    for (uint8_t i = 0; i < 8; i++) {                                  // Can safely pull it down to ground without a short circuit.      
        scl_high();                                                    
        i2c_delay_half_bit();
        data <<= 1;                                                    // Read 8 bits, MSB first
        if (sda_read()) {                                               
            data |= 1;
        }
        scl_low();
        i2c_delay_half_bit();
    }

    if (send_ack) {                                                     // Send ACK or NACK
        sda_low();                                                      // ACK: pull low
    } else {
        sda_high();                                                     // NACK: leave high
    }
    i2c_delay_half_bit();
    scl_high();
    i2c_delay_half_bit();
    scl_low();
    sda_high();                                                         // Release SDA
    i2c_delay_half_bit();
    return data;
}

// ===== HIGH-LEVEL FUNCTIONS =====
void i2c_bitbang_init(const I2C_config_t *config) {                     // Configure SCL and SDA as open-drain outputs
    current_config = *config;                                           // Use push-pull but switch between output LOW and input FLOATING
                                                                        // This mimics open-drain behavior with the pull-up resistors
    gpio_set_mode(config->port, config->scl_pin,                        // SCL: Output push-pull (always drive it)
        GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_PUSHPULL);              // SDA: switch between output LOW and input FLOATING
    gpio_set_mode(config->port, config->sda_pin,                        // Start with output HIGH (which drives it high via push-pull temporarily)
        GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOATING);                      // Use gpio_write_pin to set LOW, or switch to input for HIGH   
    gpio_write_pin(config->port, config->scl_pin, 1);                   // Ensure both lines start HIGH (idle state)
    scl_high();                                                        
    sda_high();
    initialized = true;
}

bool i2c_write(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len) {
    if (!initialized) return false;
    i2c_start();
    if (i2c_write_byte((addr << 1) | 0x00)) {                           // Send slave address (write mode)
        i2c_stop();                                                     // 0 = ACK (success), 1 = NACK (failure)
        return false;                                                   // No ACK from slave
    }

    if (i2c_write_byte(reg)) {                                          // Send register address
        i2c_stop();
        return false;
    }

    for (uint8_t i = 0; i < len; i++) {
        if (i2c_write_byte(data[i])) {
            i2c_stop();
            return false;
        }
    }
    i2c_stop();
    return true;
}

bool i2c_read(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len) {
    if (!initialized || len == 0) return false;
    i2c_start();                                                        // First, write the register address 
    if (i2c_write_byte((addr << 1) | 0x00)) {                           // tell the slave in this address that we want to write
        i2c_stop();                                                     // 0 = ACK (success), 1 = NACK (failure)
        return false;   
    }

    if (i2c_write_byte(reg)) {                                          // Write the register address that we want to read from
        i2c_stop();                                                     // the slave will set its register pointer to this address
        return false;
    }

    i2c_start();                                                        // Repeated START for read, if we stop first, other master might interrupt
    if (i2c_write_byte((addr << 1) | 0x01)) {                           // tell the slave in this address that we want to read
        i2c_stop();
        return false;
    }

    for (uint8_t i = 0; i < len; i++) {                                 // Read data bytes, from the register address that we set earlier
        data[i] = i2c_read_byte(i < (len - 1));                         // Send ACK for all bytes except the last one
    }

    i2c_stop();                                                         // Only stop at the end
    return true;
}

bool i2c_probe(uint8_t addr) {                              // In I2C communication, standard device addresses are 7 bits long (ranging from 0x00 to 0x7F) 
    if (!initialized) return false;                         // the I2C protocol mandates that the very first byte transmitted after a START condition must be 8 bits long
    i2c_start();                                            // Bit:    7    6    5    4    3    2    1    0
    uint8_t ack = i2c_write_byte((addr << 1) | 0x00);       //         [---- 7-Bit Device Address ----]  [R/W]
    i2c_stop();                                             // initiating a Write sequence              | 0x00 
    return (ack == 0);                                      // true if device responded
}

void debug_gpio_state(void) {
    usart_printf(USART1, "\r\n=== GPIO Debug ===\r\n");
    
    // Set SCL high, read back
    scl_high();
    usart_printf(USART1, "SCL after high: %d\r\n", 
                 gpio_read_pin(PORT_GPIOB, PIN_GPIO6));
    
    // Set SCL low, read back
    scl_low();
    usart_printf(USART1, "SCL after low: %d\r\n", 
                 gpio_read_pin(PORT_GPIOB, PIN_GPIO6));
    
    // Set SDA high, read back
    sda_high();
    usart_printf(USART1, "SDA after high: %d\r\n", 
                 gpio_read_pin(PORT_GPIOB, PIN_GPIO7));
    
    // Set SDA low, read back
    sda_low();
    usart_printf(USART1, "SDA after low: %d\r\n", 
                 gpio_read_pin(PORT_GPIOB, PIN_GPIO7));
    sda_high();
}

const I2C_config_t MAX30102_I2C_CFG = {
    .scl_pin = PIN_GPIO6,                                               // PB6
    .sda_pin = PIN_GPIO7,                                               // PB7
    .port = PORT_GPIOB,
    .speed_hz = 100000,                                                 // 100kHz standard mode
};

// ===== HARDWARE I2C FUNCTIONS =====
void i2c_hardware_init(uint32_t speed_hz) {
    // Enable GPIOB and I2C1 clocks
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_I2C1);
    systick_delay_ms(10);

    // DISABLE I2C
    I2C1->CR1 &= ~I2C_CR1_PE;
    systick_delay_ms(10);

    // Reset I2C
    I2C1->CR1 |= I2C_CR1_SWRST;
    systick_delay_ms(10);                                          
    I2C1->CR1 &= ~I2C_CR1_SWRST;
    systick_delay_ms(10);

    // PB6 = I2C1_SCL (Alternate Function Open-Drain)
    gpio_set_mode(PORT_GPIOB, PIN_GPIO6, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_OPENDRAIN);
    // PB7 = I2C1_SDA
    gpio_set_mode(PORT_GPIOB, PIN_GPIO7, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_OPENDRAIN);
    systick_delay_ms(10);

    // Get APB1 frequency (should be 22 MHz)
    // Configure clock
    // Calculate CCR for desired speed
    // For standard mode (100kHz): CCR = PCLK1 / (2 * speed)
    // For fast mode (400kHz): CCR = PCLK1 / (3 * speed) with DUTY bit set
    I2C1->CR2 = 22;          // FREQ = 22 MHz
    I2C1->CCR = 110;         // 100kHz (hardcoded for clarity)
    I2C1->TRISE = 23;        // Max rise time
    systick_delay_ms(10);
    // Enable I2C
    I2C1->CR1 |= I2C_CR1_PE;                                        // PE
    systick_delay_ms(10);
    usart_printf(USART1, "I2C Hardware initialized!\r\n");
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

// static bool i2c_wait_flag(volatile uint32_t *reg, uint32_t flag, bool set) {                    // Simple timeout helper
//     uint32_t timeout = 1000000;                                                                 // ~1 second at 44MHz
//     if (set) {
//         while(!(*reg & flag) && --timeout);
//     } else {
//         while(*reg & flag && --timeout);
//     }
//     return (timeout > 0);
// }

// static bool i2c_hardware_send_addr(uint8_t addr, bool read) {
//     // Send address with R/W bit
//     I2C1->DR = (addr << 1) | (read ? 1 : 0);
//     // Wait for ADDR flag
//     if(!i2c_wait_flag(&I2C1->SR1, I2C_SR1_ADDR, true)){
//         return false;
//     }
//     // Clear ADDR by reading SR2
//     (void)I2C1->SR2;
//     return true;
// }

bool i2c_hardware_write(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len){
    uint32_t timeout;
    // Wait for bus idle
    timeout = 100000;
    while((I2C1->SR2 & I2C_SR2_BUSY) && --timeout);
    if (timeout == 0) return false;

    // Generate START
    I2C1->CR1 |= I2C_CR1_START;
    timeout = 100000;
    while (!(I2C1->SR1 & I2C_SR1_SB) && --timeout);
    if (timeout == 0) return false;
  
    // Send address (write)
    I2C1->DR = (addr << 1) | 0;
    timeout = 100000;
    while(!(I2C1->SR1 & I2C_SR1_ADDR) && --timeout);
    if (timeout == 0) return false;
    (void)I2C1->SR2;

    // Send register address
    I2C1->DR = reg;
    timeout = 100000;
    while (!(I2C1->SR1 & I2C_SR1_TxE) && --timeout);
    if (timeout == 0) return false;

    // Send data bytes
    for(uint8_t i = 0; i < len; i++) {
        I2C1->DR = data[i];
        timeout = 100000;
        while (!(I2C1->SR1 & I2C_SR1_TxE) && --timeout);
        if (timeout == 0) return false;
    }
    // Wait for BTF before STOP
    timeout = 100000;
    while (!(I2C1->SR1 & I2C_SR1_BTF) && --timeout);
    // Generate STOP
    I2C1->CR1 |= I2C_CR1_STOP;
    // Wait for bus idle
    timeout = 100000;
    while ((I2C1->SR2 & I2C_SR2_BUSY) && --timeout);
    return true;
}

bool i2c_hardware_read(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len) {
    if (len == 0) return false;
    uint32_t timeout;
    // Wait for bus to be free
    // ===== Phase 1: Set register pointer =====
    timeout = 100000;
    while((I2C1->SR2 & I2C_SR2_BUSY) && --timeout);
    if (timeout == 0) return false;

    // Generate START
    I2C1->CR1 |= I2C_CR1_START;
    timeout = 100000;
    while(!(I2C1->SR1 & I2C_SR1_SB) && --timeout);
    if(timeout == 0) return false;

    // Send address (write)
    I2C1->DR = (addr << 1) | 0;
    timeout = 100000;
    while(!(I2C1->SR1 & I2C_SR1_ADDR) && --timeout);
    if(timeout == 0) return false;
    (void)I2C1->SR2;                                    // Clear ADDR

    // Send register address
    I2C1->DR = reg;
    timeout = 100000;
    while(!(I2C1->SR1 & I2C_SR1_TxE) && --timeout);
    if(timeout == 0) return false;

    // Wait for BTF to ensure register was sent
    timeout = 100000;
    while(!(I2C1->SR1 & I2C_SR1_BTF) && --timeout);
    if (timeout == 0) return false;

    // ===== Phase 2: Repeated START and read =====
    I2C1->CR1 |= I2C_CR1_START;
    timeout = 100000;
    while(!(I2C1->SR1 & I2C_SR1_SB) && --timeout);
    if (timeout == 0) return false;

    // Send address (read)
    I2C1->DR = (addr << 1) | 1;
    timeout = 100000;
    while (!(I2C1->SR1 & I2C_SR1_ADDR) && --timeout);
    if (timeout == 0) return false;
    (void)I2C1->SR2;                                                                                    // Clear ADDR

    // Read data
    for (uint8_t i = 0; i < len; i++) {
        timeout = 100000;
        while(!(I2C1->SR1 & I2C_SR1_RxNE) && --timeout);
        if (timeout == 0) return false;
        data[i] = I2C1->DR;
    }
    // Generate STOP
    I2C1->CR1 |= I2C_CR1_STOP;
    // Wait for STOP to complete
    timeout = 100000;
    while((I2C1->SR2 & I2C_SR2_BUSY) && --timeout);
    return true;
}

bool i2c_hardware_probe(uint8_t addr) {
    uint32_t timeout;
    // Wait for bus idle
    timeout = 100000;
    while((I2C1->SR2 & I2C_SR2_BUSY) && --timeout);
    if (timeout == 0) return false;
    // Generate START
    I2C1->CR1 |= I2C_CR1_START;
    timeout = 100000;
    while(!(I2C1->SR1 & I2C_SR1_SB) && --timeout);
    if (timeout == 0) return false;
    // Send address (write)
    I2C1->DR = (addr << 1) | 0;

    // Wait for ADDR or AF (acknowledge failure)
    timeout = 100000;
    while (--timeout) {
        if (I2C1->SR1 & I2C_SR1_ADDR) {
            (void)I2C1->SR2;                                    // Clear ADDR
            I2C1->CR1 |= I2C_CR1_STOP;
            // Wait for STOP to complete
            uint32_t stop_timeout = 100000;
            while ((I2C1->SR2 & I2C_SR2_BUSY) && --stop_timeout);
            return true;                                        // Device ACKed!
        }
        if (I2C1->SR1 & I2C_SR1_AF) {
            I2C1->SR1 &= ~I2C_SR1_AF;                           // Clear AF
            I2C1->CR1 |= I2C_CR1_STOP;
            uint32_t stop_timeout = 100000;
            while ((I2C1->SR2 & I2C_SR2_BUSY) && --stop_timeout);
            return false;
        }
    }
    I2C1->CR1 |= I2C_CR1_STOP;
    return false;
}

bool i2c_hardware_read_fifo(uint8_t addr, uint8_t *data, uint8_t len){
    uint32_t timeout;
    // Wait for bus idle
    timeout = 100000;
    while((I2C1->SR2 & I2C_SR2_BUSY) && --timeout);
    if (timeout == 0) return false;
    // Generate START
    I2C1->CR1 |= I2C_CR1_START;
    timeout = 100000;
    while(!(I2C1->SR1 & I2C_SR1_SB) && --timeout);
    if (timeout == 0) return false;

    // Send address (set register pointer)
    I2C1->DR = (addr << 1) | 0;
    timeout = 100000;
    while(!(I2C1->SR1 & I2C_SR1_ADDR) && --timeout);
    if (timeout == 0) return false;
    (void)I2C1->SR2;
    I2C1->DR = 0x07;
    timeout = 100000;
    while(!(I2C1->SR1 & I2C_SR1_TxE) && --timeout);
    if (timeout == 0) return false;

    // wait for BTF
    timeout = 100000;
    while(!(I2C1->SR1 & I2C_SR1_BTF) && --timeout);

    // Repeated START
    I2C1->CR1 |= I2C_CR1_START;
    timeout = 100000;
    while(!(I2C1->SR1 & I2C_SR1_SB) && --timeout);
    if (timeout == 0) return false;

    // Send address (read mode)
    I2C1->DR = (addr << 1) | 1;
    timeout = 100000;
    while(!(I2C1->SR1 & I2C_SR1_ADDR) && --timeout);
    if (timeout == 0) return false;
    (void)I2C1->SR2;                                        // Clear ADDR

    // Read data bytes
    for (uint8_t i = 0; i < len; i++) {
        timeout = 100000;
        while(!(I2C1->SR1 & I2C_SR1_RxNE) && --timeout);
        if (timeout == 0) return false;
        data[i] = I2C1->DR;
    }
    // Generate STOP
    I2C1->CR1 |= I2C_CR1_STOP;
    timeout = 100000;
    while((I2C1->SR2 & I2C_SR2_BUSY) && --timeout);
    return true;
}

void i2c_hardware_sensor_init(void) {
    uint8_t reg_val, read_back;
    // Reset
    reg_val = 0x40;
    if (!i2c_hardware_write(0x57, 0x09, &reg_val, 1)) {
        usart_printf(USART1, "Failed to write reset!\r\n");
    }
    systick_delay_ms(100);
    if (i2c_hardware_read(0x57, 0x09, &read_back, 1)) {
        usart_printf(USART1, "Mode reg: 0x%02X (should be 0x00 after reset)\r\n", read_back);
    }
    // FIFO Config
    reg_val = 0x1F;
    i2c_hardware_write(0x57, 0x08, &reg_val, 1);            // Sample avg=4, FIFO rollover
    if (i2c_hardware_read(0x57, 0x08, &read_back, 1)) {
        usart_printf(USART1, "FIFO reg: 0x%02X\r\n", read_back);
    }
    // Mode: SpO2
    reg_val = 0x03;
    i2c_hardware_write(0x57, 0x09, &reg_val, 1);
    // SpO2 Config
    reg_val = 0x27;
    i2c_hardware_write(0x57, 0x0A, &reg_val, 1);            // ADC range = 4096nA, sample rate=100Hz, pulse width=411µs
    // LED current
    reg_val = 0x24;
    i2c_hardware_write(0x57, 0x0C, &reg_val, 1);            // ~7.2mA, if your raw values are sitting below 50,000 increase it
    i2c_hardware_write(0x57, 0x0D, &reg_val, 1);

    if (i2c_hardware_read(0x57, 0x0C, &reg_val, 1)) {
        usart_printf(USART1, "LED Red (0x0C): 0x%02X (expect 0x24)\r\n", reg_val);
    }
    // Enable temp
    reg_val = 0x01;
    i2c_hardware_write(0x57, 0x21, &reg_val, 1);
    // Clear FIFO pointers
    reg_val = 0x00;
    i2c_hardware_write(0x57, 0x04, &reg_val, 1);            // Write pointer = 0
    if (i2c_hardware_read(0x57, 0x04, &reg_val, 1)) {
        usart_printf(USART1, "Write Ptr (0x04): %d\r\n", reg_val);
    }
    i2c_hardware_write(0x57, 0x05, &reg_val, 1);            // Overflow counter = 0
    i2c_hardware_write(0x57, 0x06, &reg_val, 1);            // Read pointer = 0
    if (i2c_hardware_read(0x57, 0x06, &reg_val, 1)) {
        usart_printf(USART1, "Read Ptr (0x06): %d\r\n", reg_val);
    }
    systick_delay_ms(50);                                   // Wait for ~5 samples
}

// ===== BITBANG I2C DATA FUNCTIONS =====
void i2c_bitbang_temp(void) {
    uint8_t temp_int, temp_frac;

    // Read temperature (integer part)
    if (i2c_read(0x57, 0x1F, &temp_int, 1)) {
        int8_t temp_c = (int8_t) temp_int;
        usart_printf(USART1, "Die Temperature: %d°C\r\n", temp_c);
    }

    // Read temperature fractional (0x20)
    if (i2c_read(0x57, 0x20, &temp_frac, 1)) {
        float temp = (int8_t)temp_int + (temp_frac * 0.0625f);
        usart_printf(USART1, "Precise Temp: %.2f°C\r\n", temp);
    }

    // Read interrupt status (0x00)
    uint8_t int_status;
    if (i2c_read(0x57, 0x00, &int_status, 1)) {
        usart_printf(USART1, "Interrupt Status: 0x%02X\r\n", int_status);
    }
}

void i2c_sensor_init(void) {
    // Reset the device
    i2c_write(0x57, 0x09, (uint8_t[]){0x40}, 1);            // MODE: Reset
    systick_delay_ms(100);                                  // Wait for reset
    // FIFO Configuration
    i2c_write(0x57, 0x08, (uint8_t[]){0x4F}, 1);            // Sample avg=4, FIFO rollover
    // Mode Configuration: SpO2 mode
    i2c_write(0x57, 0x09, (uint8_t[]){0x03}, 1);            // SpO2 mode
    // SpO2 Configuration
    i2c_write(0x57, 0x0A, (uint8_t[]){0x27}, 1);            // ADC range = 4096nA, sample rate=100Hz, pulse width=411µs
    // LED Pulse Amplitude (IR LED)                         // if your raw values are already above 100000 leave it
    i2c_write(0x57, 0x0D, (uint8_t[]){0x24}, 1);            // ~7.2mA, if your raw values are sitting below 50,000 increase it
    // LED Pulse Amplitude (Red LED)                        
    i2c_write(0x57, 0x0C, (uint8_t[]){0x24}, 1);            // ~7.2mA
    // Enable temperature sensor
    i2c_write(0x57, 0x21, (uint8_t[]){0x01}, 1);
    //  Wait for everything to stabilize
    systick_delay_ms(50);
}

void i2c_bitbang_fifo(void) {
    uint8_t fifo_data[6];                                   // 2 samples worth
    uint8_t write_ptr, read_ptr;
    // Read the write and read pointers to see how many samples are waiting
    if (!i2c_read(0x57, 0x04, &write_ptr, 1)) return;
    if (!i2c_read(0x57, 0x06, &read_ptr, 1)) return;

    // Calculate total unread samples sitting in the FIFO
    int num_samples = (int)write_ptr - (int)read_ptr;
    if (num_samples < 0) {
        num_samples += 32;                                  // Handle pointer rollover wrap-around
    }

    // Loop and read every single available sample, each sample: 6 bytes from FIFO (0x07)
    // Infrared light (950nm) penetrates deeply into human flesh and bone, resulting in much higher transmission back to the sensor photodiode.
    // Red light (660nm) is highly absorbed by tissue, melanin, and hemoglobin, resulting in much lower transmission.
    // if you got Red > IR, then reverse the fifo_data reading --> IR = fifo[3], fifo[4], fifo[5] | red = fifo[0], fifo[1], fifo[2]
    for (uint8_t i = 0; i < num_samples; i++) {
        if (i2c_read(0x57, 0x07, fifo_data, 6)) {
            // Parse first sample (first 3 bytes), 18 bits per data sample, last buffer hold the last 2 bits, The remaining 6 bits are padding (zeros).
            uint32_t ir_sample  = (fifo_data[0] << 16) | (fifo_data[1] << 8) | fifo_data[2]; // fifo_data[2] is the last 2 bits
            uint32_t red_sample = (fifo_data[3] << 16) | (fifo_data[4] << 8) | fifo_data[5]; // fifo_data[5] is the last 2 bits
            // MAX30102 data is left-justified, fifo_data[0] acts as the "millions/thousands" place (the most significant byte).
            // fifo_data[1] acts as the "hundreds/tens" place (the middle byte), fifo_data[2] acts as the "decimal/ones" place (the least significant byte)
            ir_sample &= 0x3FFFF;               // 18-bit value
            red_sample &= 0x3FFFF;              // 18-bit value

            usart_printf(USART1, "IR: %lu, Red: %lu\r\n", ir_sample, red_sample);
        }
    }
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